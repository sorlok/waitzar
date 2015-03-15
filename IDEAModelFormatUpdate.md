# Introduction #

The old model format was written back when I knew little about C++. (It was implemented using a massive double array of pointers, to much shuddering.) Later, I removed the pointers, but kept things like word IDs, since I was worried about using maps for jump lists, etc. Now, I think we can store data in memory in a way that is even simpler; for example, using a map of words-to-prefixes to avoid a whole bunch of array entries.

Throughout this process, the "binary-ish" style of the model hasn't been updated. I'd like to use the JSON parser (capitalizing on the full power of JSON, including numbers and arrays) to read model files, in the interest of robustness, code reuse, etc. So, this page will attempt to review both the new storage format (on disk) and its representation in memory.


# Old Storage Format #

The old storage format went something like this:
  * Several sections, each labeled with a "#ID: XXXX", where "ID" was "DEFINE", "MAP", or "SEGMENT", and "XXXX" was the number of entries (lines, usually, except in "DEFINE") in that section. Extra comments caused parse errors.
  * DEFINE contained several lines, each of the form "XXX[AA,BB-CC-DD,...,ZZ]", where "XXX" was the ID of the first word on that line (used only when manually debugging the file), and each element in the array was a word in the dictionary, separated by commas. If a word had multiple letters, these were separated by dashes. Each line contained 50 definitions. Oh, and "AA" actually meant "\u10AA", since Myanmar was assumed (and Unicode 5.2 hadn't been invented yet).
  * MAP looked something like this: "{L:DDD,L:DDD,...}". Here, "L" was a single character representing a "move" one could make from that node and "DDD" was the ID (non-hex!) of the next map node to jump to. If the letter was "~", then the ID was actually the ID of the PREFIX entry to jump to.
  * PREFIX lines were even more convoluted: "{AAA:BBB,CCC:DDD}[XXX,YYY,ZZZ]". The first bracketed list was a hash lookup; if a word with ID "AAA" was in the current n-gram then prefix id "BBB" was jumped to. This process continued until no more trigrams existed/matched, at which point the "XXX", "YYY", id words represented definitions matched at that prefix id. (All numbers are non-hex). However! when adding words, one was required to start at the _last_ matching prefix and add all words in that prefix and _each_ prefix preceding that match until all words had been added. This allowed for trigrams to re-order matched words.

As you can see, the format was haphazard and required a good deal of explaining. There's nothing wrong with it _per se_, and it had the benefit of being usable in any language with simple arrays --no need for hash tables, pointers, or even array resizing. That said, I think a more portable solution is in order, especially since this format can be abused (e.g., adding a definition that is ONLY in the list if the appropriate trigram matches, which is not what WZ intended).

...by the way, the first 10 definitions were _required_ to be the numbers 1 through 0 (in Burmese, of course). Ok, ok, enough of the old format.


# New Format: Definitions #

Assuming that we use JSON (which is all but certain, since the format is very flexible _and_ we have a small parser for it), the definitions are easy:

```
#Note that definitions (words) can still be identified by ID
{ 
  "words" : ["၁", "၂", ..., "ကို"]
}
```

There are no restrictions on what words may be stored in a dictionary.


# New Format: Lookup Table #

Two facts are clear about the Wait Zar romanization. First, there was never any slowdown due to our decision to store node lookups in an array instead of a map. Given the average length of a syllable and the number of available letters in the English alphabet, I'd expect this to be true for any language. (The JSON Spirit author notes that a typical PC begins to favor maps after 500 elements, which requires syllables 20 letters long where _each_ of the 24 English letters is equally likely at any point in the word. I'll happily reconsider if you show me a language that fits this criteria and want to implement in Wait Zar.) Second, nodes only ever had a single parent, so the integer index for each node could easily be replaced by a pointer of some kind without increasing the memory used at run-time; i.e., we could just build a map of maps of maps... if we wanted to.

There's one problem with this: currently, typing "g" presumes "aung", which only applies to Burmese. I'd like to lift this feature from the code into the model file. We'll get to that later.

Let's first consider the most direct conversion from our old format:

```
{
  "lookup" : [
    [{"a":10}, {"b",20}],
    [{"k":11}, {"~",2}],
    ...
  ]
}
```

This approach actually seems more archaic, due to all the new curly braces. We can clean it up a bit by grouping the lookup letters into a string and indexing by that into an array:

```
{
  "lookup" : [
    {"abc" : [12,23,34]},
    {"ka~" : [100,10,15]},
    ...
  ]
}
```

...but that doesn't improve much. We could try separating out the "keys" into one array and the "values" into another:

```
{
  "lookup" : [
    ["abc", "ka~", ...,],
    [[12,23,34], [100,10,15], ...,],
    ...
  ]
}
```

This is probably the best solution that uses IDs. If we choose to nest the maps recursively, we end up with:

```
{
  "lookup" : {
    "a" : {
        "b" : {...},
        "c" : {...},
        "~" : 100
    } ,
    "k" : {
        "o" : {...},
        "~" : 20
    }
  }
}
```

This is actually pretty easy on the eyes, and since models are always generated by script it wouldn't be that hard to build. We could even fold in the main prefix entry by having "~" refer to an array of words instead of a prefix offset.

```
{
  "lookup" : {
    "a" : {
        "b" : {...},
        "c" : {...},
        "~" : [100,200,110]
    } ,
    "k" : {
        "o" : {...},
        "~" : [12]
    }
  }
}
```

Each node with a "~" will ALWAYS have at least one entry, so we aren't wasting any space. Note that we can't replace word IDs with actual words because (#1) prefix entries index words multiple times and (#2) a word can appear under different romanizations and should still be considered the "same" word. (Or, rather, we can't do it lightly, without a lot of thought.) Imposing a total order on words is impossible when hash tables are involved, so any clever tricks with total ordering won't help eliminate the need for a word list.



# New Format: Prefix Entries #

Ok, we're definitely getting somewhere. What about prefix lookups? Here I think we can be clever: note that prefix lookups are always invoked for romanizations, not the actual words. So, how about a hash table?

```
{
  "ngrams" : {
    "kote" : [
        [100 , [200, 100]],
        [200 , [100, 200]],
        [300 , [100, 200]]
    ] ,
    ...
  }
}
```

Here, we say that looking up "kote" will reoder if a 100, 200, or 300 precedes it. Unfortunately, the syntax gets messy for n-grams with n>1, like so:

```
{
  "ngrams" : {
    "kote" : [
        [[100,200,300] , [200, 100]],
        [[200] , [100, 200]],
        [[100] , [100, 200]]
    ] ,
    ...
  }
}
```

We can't use an array as the index in a hash table, so we'd have to search through each array to find the _most_ matching one. This is a problem for "100", since it matches both "100" and "100,200,300". Moreover, in the array "100,200,300", does 100 represent the first word in the n-gram or the last? Our decision to use word IDs is slowly killing us.

Let's consider: what happens if we use the actual words as prefix entries. (We still use word IDs for "our" definitions, so the node lookup doesn't change.) Well, the format suddenly becomes a lot simpler, thanks in part to hash lookups. While we're at it, let's nest n-grams for easier semantics:

```
{
  "ngrams" : {
    "kote" : {
      "ကို" : {
         "၁" : {...},
         "၂" : {...},
         "~" : [100, 200]
      }
    } ,
    ...
  }
}
```

So "kote" with "ကို" preceding it orders 100 before 200, unless "၁" or "၂" is also preceding it. It's pretty understandable, and not too verbose, since it only lists relationships that actually exist.


# "Aung" shortcut #

First, here's an easy cop-out:

```
{
  "shortcuts" : [
    {"ag" : "ung"},
    {"g" : "aung"}
  ]
}
```

We list "ag" first to pick up the times when a "g" is completing "aung" directly ("a"+"g", "ka"+"g"), and "g" second to catch all other cases where "g" is adding the "aung" in its entirety (e.g., "kaung", "maung").

This is a cop-out because it _should_ be in a hash table, but requires an array for total ordering. Moreover, it doesn't capture the semantics that "g" does nothing if the model already defines a mapping for "g".

We can manually insert "g" into the nexus list in all places where "aung" would match, but that won't work with our prefix lookup since the romanization string wouldn't actually be "aung". We could add a special symbol, "!" to fix that:

```
{
  "lookup" : {
    "a" : {
        "b" : {...},
        "c" : {...},
        "~" : [100,200,110],
        "!" : ["g", "ung"]    #"g" appends "ung" if nothing matches
    } ,
    "k" : {
        "o" : {...},
        "~" : [12],
        "!" : ["g", "aung"]   #"g" appends "aung" if nothing matches
    }
  }
}
```

Semantically this is fine, but it disorganizes our model. Besides being messy, consider what would happen if someone added, e.g., "xaung" to the model afterwards (using the wordlist). You'd have to detect that "xg" should now have a "!" entry, as should "xag". Alternatively, you could put a "!" entry in every single nexus node, but that would be needlessly space-inefficient.

No, what's needed is some kind of "rule" that tells the system how to proceed when matching otherwise fails --like a "default" switch handler, or a "last chance" kind of recovery method. The ideal case is a regex which acts on the entire word:

```
{
  #Not a cop-out, because it represents exactly what we mean.
  "lastchance" : [
    "/^(.*)a?g$/$1aung/"
  ]
}
```

Currently, however, it only needs to be strong enough for Burmese, as complex patterns for other languages can be added later (after C++0x is fully standardized with built-in regexes). So how about this:

```
{
  #Tiny cop-out, but justifiable.
  "lastchance" : [
    "a?g=aung"
  ]
}
```

Here, we accept only letters and "?", which means "optional". The entire match is replaced with the RHS of the equals, and matching only succeeds at the end of the string. (So, "agk" doesn't expand to "aungk", which is true for all words in WZ currently. Maybe some pat-sint words have this property...?) Also, the "=" syntax can be detected and expanded into a full regex without losing backwards compatibility, so we're not removing the possibility of leveraging regexes later.


# Easy Pat-Sint #

Actually, there's more to the model than these 3 sections. "Pat-sint" shortcuts really show the old format's weakness, as can be seen from a snippet of the "easypatsint" file:

```
#### --stacked ဍ
  #ဘဏ် + ဍာ = ဘဏ္ဍာ    #Can't type component
  #သဏ် + ဍာန် = သဏ္ဍာန် #Can't type component
  #သဏ် + ဍန် = သဏ္ဍန်   #Can't type component
  #ဒဏ် + ဍာ = ဒဏ္ဍာ     #Can't type component
  ကဏ် + ဍ = ကဏ္ဍ  

#### --္ဘ
  ကမ် + ဘာ့ = ကမ္ဘာ့
  #ကမ် + ဘ = ကမ္ဘ    #NEXUS collision
  ကမ် + ဘာ = ကမ္ဘာ
  ကမ် + ဘော = ကမ္ဘော
```

The "can't type component" warnings mean that either the left-hand-side or the right-hand-side can't be typed. There is no way of re-enabling commented lines, so if "ဍာန်" suddenly became (somehow) type-able, then "သဏ္ဍာန်" would not magically appear as a shortcut. The "nexus collision" error means that the "ba" (ဘ) in "kamba" has the same romanization as "ba" (ဘာ့). This is such a messy error, because if we were to change the romanization of "ဘာ့" to, say, "ba." (which is how it would look in Romabama), then our nexus collision would disappear --maybe leading to a valid parse tree or maybe appearing elsewhere as an error. The point is, such a system is untenable.

To represent pat-sint shortcuts better, let's first observe that the "previous word" of a pat-sint pair can refer to various different combinations depending on the next candidate. The problem is, the first word of a pair is indexed by Myanmar word, while the second is indexed by romanization (since we type "ကမ်" and then "BA", not "ကမ်" and then "ဘာ့" or "ဘ". So we've got a clear conflict of interests.
  * Either we make a list of {"ကမ်" : {"ဘ" : "ကမ္ဘ", "ဘာ့" : "ကမ္ဘာ့", ...}, ...}, and when the user types "ba" we take each _candidate_ for "ba" and check if it exists as an entry in the hashmap for "ကမ်", thus complicating lookup...
  * ...or, we define a new "shortcut" character like "?" and add an entry for "ကမ်" in the nexus of "ba", with ["ကမ္ဘ", "ကမ္ဘာ့"], etc., and we lose flexibility in maintaining this lookup list....
  * ...or, we define a myanmar/roman mapping lookup of {"ကမ်" : {"ba" : ["ကမ္ဘ", "ကမ္ဘာ့", ...], ...}, ...}, and have to edit this list on every romanization change.

The problem is that the current "X" + "Y" = "Z" format really defines the mappings perfectly, but falls apart when romanization comes into play, since either "X" or "Y" may not actually have a romanization. Options 2 and 3 hide the nature of pat-sint words and allow errors to creep in to the model whenever an update is performed, so we're left with option 1. Expanded, it looks like this:

```
{
  "shortcuts" : {
    "ကမ်" : {
      "ဘ" : "ကမ္ဘ",
      "ဘာ့" : "ကမ္ဘာ့",
      "ဘော" : "ကမ္ဘော"
    },
    ...
  }
}
```

The matching process will now have to scan through every potential candidate, since this list contains no romanization information, but it's the simplest solution that doesn't wildly distort the semantics. It also supports the ability to produce a "no frills" (myanmar = roman) model simply by lopping off the tail end of a model file ("shortcuts", "lastchance", and "ngrams", that is). In other words, a beginner writing a model for, say, Chinese could safely ignore these sections.


# Putting it Together #

Here's a prototypical config file for a few small words. Yay, hand-written models!

```
#Demonstration model
{
  #Words our model supports: "ko", "kaung", "kan", "ba", "kamBa"
  "words" : [
    "ကို", "ကိုယ်",
    "ကောင်း", "ကောင်",
    "ကမ်", 
    "ဘ", "ဘာ့",
    "ကမ္ဘာ", "ကမ္ဘာ့"
  ],


  #How to type these words
  "lookup" : {
    "b" : {
      "a" : {
        "~" : [5, 6]
       }
    },
    "k" : {
      "o" : {
        "~" : [0, 1]
      },
      "a" : {
        "u" : {
          "n" : {
            "g" : {
              "~" : [2, 3]
            }
          }
        },
        "n" : {
          "~" : [4]
        },
        "m" : {
          "b" : {
            "a" : [7, 8]
          }
        }
      }
    }
  },

  #Prefixes: "ko" after "kan" chooses the 2nd option unless "ba"
  "ngrams" : {
    "ko" : {
      "ကမ်" : {
         "ဘ" : {
           "~" : [0, 1]
           },
         "~" : [1, 0]
      }
    }
  },

  #Repair function: "g" as "(a)ung"
  "lastchance" : [
    "a?g=aung"
  ],

  #Easy pat-sint: "kan" + "ba" = "kamba"
  "shortcuts" : {
    "ကမ်" : {
      "ဘ" : "ကမ္ဘ",
      "ဘာ့" : "ကမ္ဘာ့"
    }
  }
```


# Observations #

Some observations:

  * The new model doesn't make any of the "hard" concepts (n-grams, pat-sint (if you're not Burmese)) any easier to understand, but that's to be expected.
  * The model requires a **LOT** less to be explained. However, a few things still need explaining:
    * The "lastchance" syntax.
    * The fact that you can't use multiple letters in the "lookup" tree.
    * The use of "~" to mean "here's our match" in the lookup tree.
    * Indexing words by ID.
    * How "ngram" reordering works.
  * The model can store non-active information (e.g., pat-sint words whose components can't be typed) which may become active later (e.g., a "mywords" file that contains those components). The model can potentially store all pat-sint words now as combinations.
  * The model will almost definitely take up more space than the previous one. How much space, exactly, remains to be seen.
    * The "automated" parts of the model file (words, prefixed, nexi) do not seem to require much more information to store. (Prefixes should require less). The "manual" parts (pat-sint, aung shortcut) hopefully won't be a problem since they are added manually.

Overall, I like the new format. Time to get coding! First up is a script for converting our existing model file to the new json format, next is a set of C++ classes for holding the result, third is a simple mapper from JSON to this new format.


# Implementation Notes #

The script to convert old model files will most likely be in Python, which has decent (a) Unicode support and (b) json support. For example, here's how to save a file with Unicode literals:

```
  #Random init stuff
  import json
  import codecs
  test = {'hi' : u'\u1000'}  #Use [] for arrays, {} for objects

  #Save
  f = codecs.open('out.txt', encoding='utf-8', mode='w')
  f.write(json.dumps(test, ensure_ascii=False))
  f.close()
```

There are a whole bunch of options for pretty-printing and terse-printing.

There's a problem with prefixes. It's very common for a word to have ONLY a unigram, which leads to the overly-verbose syntax:

```
  "yan": {
    "ဂ": {
      "~": [1937]
    },
    ...
```

We want something more like this:

```
  "yan": {
    "ဂ": [1937],
    ...
```

...with the "full" syntax only required for items with multiple children. Another option is to sacrifice some semantic clarity and have only a single layer of depth:

```
  "pyaw": {
    "တော့": [1334,1305],
    "တော့/ဆက်" : [1305],
    "တွေ" : [1334,1305],
    ...
```

...here we use a "/" to signify multiple prefixes. Then, when searching for ['prefix1', 'prefix2', 'prefix3'], we try matching 'prefix1/prefix2/prefix3', then 'prefix1/prefix2', then 'prefix1'. The "n" in "n-gram model" rarely gets to the size where this becomes unwieldy. I think we'll do it!