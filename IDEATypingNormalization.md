# Encoding #

Myanmar text in this wiki page is encoded in Unicode. Please [check this page](http://www.myanmarlanguage.org/unicode/myanmar-fonts-which-follow-unicode-rules) for a list of Unicode-enabled fonts. (We are currently in the process of switching all of our wiki pages to use Unicode).


# Introduction #

The Zawgyi-One keyboard contains about 130 rules for normalizing typed text, and even then it still cannot type some words properly. The IDEA article on Key Magic improvements could probably shorten this list down to 50 or 60 rules, but they would still be fairly complex. In fact, the entire solution can be described in two lines:

**Typing order**:  ေ ြ က ္က/-င်္  ျ/ွ/ှ/ိ/ု/ာ/ံ/့/်/-း

**Encoding order**:  -င်္ က ္က  ျ  ြ  ွ  ှ  ေ  ိ  ု  ာ  ံ  ့  ် -း

Here, "က" stands for "any consonant" and "္က" for "any stacked consonant". Slashes mean that a letter can by typed in any order. For example, some users type "ိ" before "ု", but actually the reverse is ok. The encoding, of course, is much stricter than the typing order.

The lines printed above aren't 100% complete; for example, "ီ" can be typed instead of "ိ". And what do you do about obvious errors, like "ိ" + "ီ" (which overlaps)? Still, these two lines adequately describe a huge part of the problem. One sample rule in the Zawgyi keyboard file illustrates how complicated this can get.
```
   $consU[*] + $yayit + $twh + U1039 + $consU[*] + $kinzi + $afterKinzi[*] => $6 + $1 + $2 + $3 + $4 + $5 + $7
```

Can we translate our two-line solution into something fast, efficient, and equally easy to understand?


# General Description of Solution #

Consider a series of boxes. Each box has a tab above it and a tab to the right. The tab above specifies which letters it can contain. The tab to the right specifies how many letters it can hold, and whether or not the letters it holds must be unique. For now, consider "င်္" and "္က" to be single letters.

Here are the diagrams of our "typing order" and "encoding order" for the Zawgyi keyboard, slightly modified to be more realistic. Note that "encoding order" does not have tabs to the right; this will be explained later.

![http://waitzar.googlecode.com/svn-history/r1392/trunk/img_guide/wiki_zg_typing_order.png](http://waitzar.googlecode.com/svn-history/r1392/trunk/img_guide/wiki_zg_typing_order.png)

![http://waitzar.googlecode.com/svn-history/r1394/trunk/img_guide/wiki_uni_encoding_order.png](http://waitzar.googlecode.com/svn-history/r1394/trunk/img_guide/wiki_uni_encoding_order.png)

(TODO: We forgot "ါ")

One must type a word by starting at the **left** side of the typing order, and either staying at the current box or moving right. If, for example, one types "ြ" then "က",then the letters "ြ" and "ေ" cannot be typed (because they would force us to move to the left) and neither can we type another "က" (because the box for "က" can only contain 1 entry). Likewise, after typing "း" there is no way to continue typing without moving left.

Note that we _might_ choose to group stacked letters with the other "modifiers". For example, when typing "သင်္ချိုင်း", some people might type "င်္" after they type "ိ".

As we type each letter, we match it in the "typing string" (as described above) and we also add it to the "encoding string". So, after typing "ြ", we save it in the fifth box in the encoding order. If we typed "ေ", it would appear in the eighth box, regardless of whether we typed it before "ြ" or after it. If multiple letters are typed into the same box (in encoding order), we just append them. So, if the user types "ု" then "ူ", we store "ု" and then append "ူ" to it. Even though "ု"+"ူ" is an invalid sequence, it is still "type-able", and we want to allow it to be typed (even if we later provide the option to remove it). The reason for this is that some encodings (e.g., Ayar, Zawgyi) are not as strict as Unicode, so we want our underlying system to be flexible.

After typing the following letters: ("ြ" + "က" + "ံ" + "ု" + "း"), our diagram ends up looking like this:


![http://waitzar.googlecode.com/svn-history/r1395/trunk/img_guide/wiki_zg_typing_order_example.png](http://waitzar.googlecode.com/svn-history/r1395/trunk/img_guide/wiki_zg_typing_order_example.png)

![http://waitzar.googlecode.com/svn-history/r1395/trunk/img_guide/wiki_uni_encoding_order_example.png](http://waitzar.googlecode.com/svn-history/r1395/trunk/img_guide/wiki_uni_encoding_order_example.png)

When it is impossible to continue typing without moving left, we then "flush" the system. Each box in the encoding order is read left-to-right, and its contents are flushed to the typed string. (If a box contains multiple items, flush them in the order they were typed). Then, reset to the left-most box and resume typing. Note that a temporary flushing of the system can also be used if we want to display a partially-typed string.

Note that this "flushing" and "moving right" happens behind the scenes. The user should not be aware of it. In order to continue using "smart backspace", we will have to save the state of the "typing" and "encoding" arrays at each step, along with the state of the "typed sentence string" (which is currently the only thing Backspace needs to save and restore).

One open issue is with characters like U+1039, which will have meaning later (as part of a "stacked letter") but can sometimes be typed alone (e.g., with Myanmar3). How do we order these while waiting?


# Programming a Representation #

The boxes for "how many letters" a box can hold, and "must they be unique?" are not really necessary. Although they describe something valuable about the language, they only really affect the typing of invalid sequences, which is partly outside our concern.

Thus, we can reduce each ordering to an array of strings. Using Key Magic syntax, we might represent them as:

```
   $consonant = U[1000..1021]
   $yy_twh = U103C + U1031
   $stackedConsKinzi = ["\u1039\u1000" + "\u1039\u1001" + /*manually expand*/ + "\u1004\u103A\u1039"]
   $modifier = U103B + U103D + U103E + U102D + U102E + U1032 + U102F + U1030 + U102C + U102B + U1036 + U1037 + U103A
   $final = U1038
   NORM_TYPING = [$yy_twh[*] + $consonant[*] + $stackedConsKinzi[*] + $modifier[*] + $final]


   $stackedCons = ["\u1039\u1000" + "\u1039\u1001" + /*manually expand*/]
   $yayit = U103C
   $twh = U1031
   $yc = U102B + U102C
   $kinzi = U1004 + U103A + U1039
   $upperVowel = U102D + U102E + U1032
   $lowerVowel = U102F + U1030

   NORM_ENCODING = [$kinzi + $consonant[*] + $stackedCons[*] + U103B + $yayit + U103D + U103E + $twh + $upperVowel[*] \
      + $lowerVowel[*] + $yc[*] + U1036 + U1037 + U103A + U1038]
```

Although this is still a bit wordy, it represents the normalization problem a lot better than a series of replacement rules, and it should be fairly quick to parse (certainly quicker than +100 match rules).


# Use Outside of KeyMagic #

We might consider specifying this via some other format. For example, the NORM\_ENCODING will always be the same for Unicode. And the NORM\_TYPING will (I think) be the same for myWin2.2 and Myanmar3, as well as for Ayar and Zawgyi-One (although Ayar has a different NORM\_ENCODING). The point is, this might be better as a config-level mechanic.

That would also allow us to use the normalization rules on myWin2.2, which is not technically a KeyMagic keyboard --although I'm considering making it one just to make things easier.

Anyway, we've got plenty of technical problems to solve first, before we actually implement this. So there's plenty of time left to think about it!