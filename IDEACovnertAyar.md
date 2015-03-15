# Introduction #

The Ayar Unicode project is another non-compliant encoding of Unicode:
http://code.google.com/p/ayarunicode/

However, it has some distinct advantages over the other "partial Unicode" encodings:
  1. It has a very open license (MIT).
  1. It is under active development (unlike Zawgyi 2010).
  1. It looks nice (as a system font).
  1. It does not use minority code points (like Zawgyi-One, Mya Zedi).
  1. Shan/Mon/etc. fonts are under development.
  1. The encoding is fairly sensible; definitely Unicode-inspired. (So, we could conceivably use it as a BMP font with some hacking.)

Given these advantages, the main problem is creating a Transformation. I'd like Ayar to be the first example of a user-written Javascript converter; however, I also want a demo in 1.8. So, we'll just do a simple one for this release.


# Unicode 2 Ayar #

  * First, scan for syllables (remember that Kinzi comes BEFORE the consonant).
  * Then, for each syllable:
    * Put kinzi (1004) (103A) (1039) after the consonant.
    * Put ေ(1031) or ျ (103C) before the consonant.

# Ayar 2 Unicode #

  * First, scan for syllables (use the Zawgyi method of finding them).
  * Then, for each syllable:
    * Put kinzi (1004) (103A) (1039) before the consonant.
    * Put ေ(1031) or ျ (103C) after the consonant.
