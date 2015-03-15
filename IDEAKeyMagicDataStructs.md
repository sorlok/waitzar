# Introduction #

KeyMagic was introduced as a free keyboard management system, and its format is fairly concise. Originally, we were going to use a parser (like boost::spirit) to read config files, but since we might drop Boost support in 1.9, I decided to write a very simple tokenizer myself. Details of Key Magic & its formats can be found on the project page:
http://code.google.com/p/keymagic/


# Step 1: Pre-Processing #

WaitZar generally supports UTF-8 in everything, so keymagic source files are first converted from a bytestream to a wide-character stream. The pre-parsing step then takes this wide-character stream and performs the following operations:
  * Removes all spaces, tabs, and newlines.
  * Removes all comments, of the form /**...**/ and //...
  * Combines lines ending in \ with the following line.
The goal of this pre-processing step is to create a single list of strings, where each string is **one "line"** in the file. Even if a "line" spans multiple visible lines in the file, it will only be seen as a single "line" after pre-processing.


# Step 2: Tokenizing #

Given a list of lines, each line can then be processed in isolation. Given a line, we determine the following information:
  * The "tokens" are individual commands, separated by +, =, or =>
  * The "type" of that line is either "variable" or "replacement". This is determined by the command separator: = or =>
So, given the following line:
> $row1U = U1041 + U1042 + U1043 + U1044 + U1045 + U1046 + U1047 + U1048 + U1049 + U1040 + $ZWS
...we can say that the "type" is "variable" and the "tokens" are:
  * ["$row1U", "U1041", "U1042", "U1043", "U1044", "U1045", "U1046", "U1047", "U1048", "U1049", "U1040", "$ZWS"


# Step 3: Type Interpretation #

Given a list of tokens (for a given line), the parser now attempts to determine the type of each token. It then saves some data for each type. Data are saved into a simple struct; a list of these structs is returned:
> `struct Rule {`
> `   RULE_TYPE type;`
> `   std::wstring str;`
> `   int val;`
> `   int id;`
> `};`
The "type" of each struct determines what the values of "str", "val", and "id" actually mean. The following is a list of all types
| **Type** | **Example** | **"str" value** | **"val" value** | **"id" value** |
|:---------|:------------|:----------------|:----------------|:---------------|
| STRING | `'test', "test", U1000, null, VK_KEY_T` | The value of the string.  | N/A | N/A |
| WILDCARD | `*` | The wildchar character, `*` | N/A | N/A |
| VARIABLE | `$varX` | The variable's name: $varX | N/A | A unique ID for this variable |
| MATCHVAR | `$2` | N/A | The ID of the backreference: 2 | N/A |
| SWITCH | `($zg_on)` | The switch's name: $zg\_on | N/A | A unique ID for this switch |
| VARARRAY | `$varArray[3]` | The variable's name: $varArray | The id within the array: 3 | A unique ID for this variable |
| VARARRAY\_SPECIAL | `$varArray[*]` or `$varArray[^]` | The variable's name: $varArray | The character-value of the special character: `'^'` or `'*'` | A unique ID for this variable |
| VARARRAY\_BACKREF | `$varArray[$4]` |  The variable's name: $varArray | The id of the back reference: 4 | A unique ID for this variable |
| KEYCOMBINATION | `<VK_SHIFT & VK_KEY_T>` | N/A | Value of the key combination | N/A |

Some notes:
  * For the STRING type, surrounding quotes are stripped and escape sequences are converted. null is converted to the empty string, UXXXX is converted to '\uXXXX', and virtual key codes are converted to their character values. In addition, multiple STRING types in a row are combined into a single instance. So, `["q" + 'w' + VK_KEY_E + '\u0052' + U0054 + "y"]` becomes simply `["qwERTy"]`. Note that strings on the LHS of equations do NOT combine; otherwise patterns like `"abc" + "def" => $2 + $1` would fail. However, the RHS of equations can always merge; for example: `"abc" => "abc" + "def"`.
  * A certain amount of validation is performed at this step, to ensure that variables are not used before declared, etc.
  * The value of a key combination is simply the value of its FINAL virtual key, with the following flags "or"-ed onto it, depending on the remaining virtual keys:
    * 0x10000 for any of the SHIFT keys.
    * 0x20000 for any of the ALT keys.
    * 0x40000 for any of the CTRL keys.
    * 0x80000 for the CAPS LOCK key.
    * (Any other key cannot be used as a modifier).


# Step 4: Caching #

Any observant readers will note that we perform a great deal of computation just to load one file. In practice, this does not slow down the program; however, we are still interested in speeding things up. Generally speaking, the following information must be cached:
  * A list of variables' values (the names of the variables no longer matter, as they are addressed by id.
  * The total number of switches.
  * For each "replacement", the following:
    * A list of "match" expressions.
    * A list of "replace" expressions.
    * A list of which switches must be on for matching to be considered valid.

WaitZar caches any "compiled" KeyMagic keyboards into the shared application directory. In addition to the structures listed above, it also saves a **hash** value which represents the MD5 checksum of the original source file. If that checksum changes, WaitZar will re-generate and re-cache the compiled file. This behavior can be overridden in the config file --one good reason for doing so is that compiled keyboard files do not contain the original "replacement" texts, which would make debugging difficult for developers.


## Binary Format for Compiled Keyboard ##

Compiled keyboards are stored in binary files in the shared application directory. The file name is the fully-qualified path of the keyboard file with redundant elements removed and the "bin" extension appended. For example, given:
```
   {"languages.myanmar.input-methods" : {
     "ayar" : 
     {
       "display-name" : "Ayar Keyboard",
       "encoding" : "ayar", 
       "type" : "keymagic",
       "keyboard-file" : "AyarFile.kms"
     }
   }}
```

...the compiled file would be named:
> `myanmar.ayar.ayarfile.bin`

The file's contents are divided into a fixed-length header and a variable-length body.


## Binary Format: Header Specification ##

The header is organized as follows. "Size" entries are listed in bytes.

| **Size** | **Name** | **Description** |
|:---------|:---------|:----------------|
| 1 | Version | Version of the binary format, currently "1". If the version does not match the version stored internally to WaitZar, the bin file will be re-generated. |
| 2 | BOM | Will be 0xFEFF. If the BOM appears as 0xFFFE, then the file was encoded backwards. |
| 16     | Checksum | If the MD5 checksum of the source file does not match this value, the bin file must be re-generated. |
| 2 | Switches | Number of switches in our file. Limit: 65535 |
| 2 | Variables | Number of variables in our file. Limit: 65535 |
| 2 | Replacements | Number of replacements in our file. Limit: 65535 |


## Binary Format: Body Specification ##

Following the header is a body consisting of **Variables** + **Replacements** number of entries. Variables and replacements are stored in the same data structure, detailed below. Variables will always have **Switches\_Size** and **LHS\_Size** set to zero. Note that, in general, where an integer is expected, 0xFFFF represents -1 or "invalid", and all other values represent unsigned integers.

| **Size** | **Name** | **Description** |
|:---------|:---------|:----------------|
| 2 | Switches\_Size | Number of switches this rule requires. |
| 2 | LHS\_Size | Number of Rules on the LHS. |
| 2 | RHS\_Size | Number of Rules on the RHS. |

Following this mini-header are **Switches\_Size** entries of the form:
| **Size** | **Name** | **Description** |
|:---------|:---------|:----------------|
| 2 | Switch ID | The ID of the switch this replacement expects. |

Following the switch entries are **LHS\_Size** + **RHS\_Size** entries of the form:
| **Size** | **Name** | **Description** |
|:---------|:---------|:----------------|
| 1 | Rule Type | Zero-indexed: {STRING, WILDCARD, VARIABLE, MATCHVAR, SWITCH, VARARRAY, VARARRAY\_SPECIAL, VARARRAY\_BACKREF, KMRT\_KEYCOMBINATION} |
| 2 | Rule Value | The "value" field of a rule. |
| 2 | Rule ID | The "id" field of a rule. |
| 2 + X | Rule String | First 2 bytes specify the size of the string; remaining 2\*X bytes represent X wide characters. Letters outside the BMP not supported. The rule string is ONLY saved for STRING type variables. |


It is imperative to remember that the binary format expects minimal error checking to be performed.



# Step 5: Matching (Simple) #

I haven't looked at the Key Magic source, but I imagine that it expands all variables into one long string and then performs something like String.find() --with added considerations for back references, of course.

**NOTE:** It would be prudent to mention at this point that matches are only valid if they occur at the END of a string. For example, if the user types:
> `'abc' <VK_KEY_D>`
...then the following will match:
```
    'c' + <VK_KEY_D> ==> $1
    'cd' ==> $1
```
...but the following will not:
> `'ab' ==> $1`

I prefer an approach somewhat similar to traditional regular expression parsers. We'll use the "dot syntax" of these approaches: consider, at all times, that "•" means "the location we're currently at in the string or pattern". For example, given the string:
> `abcdef`
...the dot begins in the following location:
> `•abcdef`
No more matching is possible once we reach the end:
> `abcdef•`
...and when the dot is anywhere else, it means we are between considering these letters. So, the following means "we've just matched c, and are preparing to match d"
> `abc•def`
Finally, there is one case where matching occurs after the dot has reached the end of the string. Later, we'll see that rules _also_ have dots that move through the rule. If the dot of any rule is located before a **KEYCOMBINATION** rule, then we can move the dot one final time if that combination matches the current-typed-key exactly. This type of move can only occur once per input string.

At each "step", the dot MUST move one character to the right. If, for a given rule, the dot cannot be moved, then that rule is not a match. We maintain a list of which rules currently match; each of these rules also has its "dot". For example, if we have the following rule:
> `"abc" => //Something`
...then the dot simply begins before "a" and progresses through to "c". For other types of rules, the dot transitions between all items in the rule list. So, for:
> `'h' + $test[*] + <VK_KEY_R> => //Something`
...the dot progresses as follows:
  * `•'h' + $test[*] + <VK_KEY_R>`
  * `'h' + •$test[*] + <VK_KEY_R>`
  * `'h' + $test[*] + •<VK_KEY_R>`
  * `'h' + $test[*] + <VK_KEY_R>•`
We already know that rules are removed if they cannot move their dots forward, which leads us to the following (important) point:
> The _first_ rule candidate to move its dot all the way to the right is considered to _match_ the input string; at this point, **no other rules are considered.**

Each rule also maintains a list of string/integer pairs which refer to the back references. In general, the following holds:
  * String literals are grouped directly. So, `'abc' + 'def'` will generate the match groups `["abc", "def"]`
  * The wildcard variable arrays store an index, when appropriate. So, `$letters[*] + $whitespace[^]` might lead to the groups `[("c",3), "a"]`. In this example, note that it doesn't make sense to store an index for the `^` wildcard.
  * Virtual Keys generally store the character of their base keystroke, although one should probably not rely on this behavior.

As mentioned, each step of the algorithm "moves the dot". Before this occurs, an empty copy of the rule is added to the list of candidates. So, if we are at:
> `abcd•efg`
...and the rule we're checking is
> `"ef" + $g => //Something`
...then the following candidate is added to the list:
> `•"ef" + $g`
This allows us to match a rule even if it does not occur at the beginning of the string.

Following this, it is fairly simple to determine what rules will "allow" the dot to move:
  * **STRING** -- If the character after the dot matches this rule's character after the dot, advancing is allowed.
  * **WILDCARD** -- Advancing is allowed if the character after the dot is between 0x21 and 0x7D (ascii non-control characters) or if it is between 0xFF and 0xFFFF (unicode BMP).
  * **VARIABLE** -- The dot is first moved inside the variable (see below).
  * **SWITCH** -- The dot moves if the switch is on.
  * **VARARRAY** -- The dot moves if the character at the vararray's index matches the character after the dot.
  * **VARARRAY\_SPECIAL** -- The move is allowed if the character after the dot is _any_ (`*`) character in the vararray, or if the character is _not_ (`^`) listed in the vararray.
  * **KEYCOMBINATION** -- The move is allowed if the current keypress matches the listed combination exactly. Note that, for the rule `$abc + <VK_KEY_T> => //Something`, pressing **Shift** and **T** will not be considered a match. Special considerations are made for the capslock key, but this will be described elsewhere.
  * **MATCHVAR** and **VARARRAY\_BACKREF** never occur on the LHS of an equation, so they cannot ever be matched.


# Step 5: Matching (Complex) #

Variables complicate this. What happens if we encounter something like this:
> `"abc" + •$temp => //Something`
...with the dot located directly before the variable? In this case, before considering matches we first move the dot _inside_ the variable. We then push the new dot location onto a "stack" for the current consideration. That way, when the current variable's dot reaches the end of that variable, we can pop the stack and continue matching until the last level has been removed from the stack.

This approach allows us to handle very large variable strings without wasting space. For example, consider the following rule set:
  * $alpha = "abcdefghijklmnopqrstuvwxyz"
  * $many = $alpha + $alpha + $alpha + $alpha + $alpha + $alpha + $alpha + $alpha + $alpha + $alpha
  * $even\_more = $many + $many + $many

Altogether, such an example is unlikely to become pathological. In fact, we actually reference variables in this way as part of our preparations for more complex patterns, as described below


# Benefits #

Our approach is complex now, but by mirroring the techniques used by regular expression engines, we allow easy extension of KeyMagic to handle more complex expressions. For example, consider a hypothetical rule type, **REPEATCHAR**, which might be used like this:
> `'abc' + 'd'* => //Something`
Such a rule would match "abc", "abcd", "abcdd", etc. Our system could handle this easily. Consider the dot is before the repeat character:
> `'abc' + •'d'*`
If "d" matches the input string's next letter, we can either skip "d" entirely or we can match it multiple times. So, we first generate a rule which is a copy of the current one, and add it to the list of replacements under consideration. Then we move the dot forward on the current rule, leaving two rules in the database:
  * `'abc' + •'d'*`
  * `'abc' + 'd'*` •
If "d" is not the next letter in the input string, we _must_ move the dot. Thus, for the input string:
> `abcdde`
...we will generate two new rules as the dot passes each "d", and then all three rules will collapse into one once the "e" is reached. This also works with multi-letter repeats like:
> `'def'*`
...which allows us to easily avoid "trick" input strings like:
> `abcdefdeg`
...which should match with the dot like so: `abcdef•deg`, even though the `'def'*` pattern wants to try to continue.


# About Replacement #

Replacement is not detailed here; it should be fairly obvious given all of the above information. If any major issues are encountered while developing the KeyMagic4WaitZar module, they will be reported here:
  * **Order of Replacement** -- Rules are matched one-by-one; however, if a rule matches then the rulebase is scanned for once more for matching rules. This continues until no further rules match the input string. (It has not yet been stated if the remaining rules are matched _before_ the rulebase is re-scanned or not.)
  * **Single-ASCII Rule** -- KeyMagic has a rule for replacements of **one character** in length, if that character is within the range `(0x20 < x < 0x7F)`. After matching and replacing this one letter, the matching process terminates and returns whatever results it has found.
  * The Single ASCII rule was implemented to prevent infinite loops. However, it's pretty easy to generate infinite input anyway. Consider:
    * Rule 1: `'abc' => 'def'`
    * Rule 2: `'def' => 'abc'`
    * Rule X: `'a' => $1 + 'aa'`
  * I have two solutions for this:
    * Restrict the matching phase to, say, 500 matches. After that point, terminate with an error message, and write the offending string + input to the log file, so that users can debug on their own.
    * Add a flag to the KeyMagic comment syntax which is named `single-pass`. If this flag is on, the keyboard only makes ONE pass through the ruleset, matching as many rules as possible. I think many developers wouldn't mind this restriction. We can also add "identifiers" for rules to allow some degree of repetition. Consider, for example:
      * `RemoveA{* + 'a' + * => $1 + $3}`
      * `//Match some more rules`
      * `$temp + 'b' => $temp + 'a' + 'b'`
      * `RemoveA() //This will try to match RemoveA one more time.`