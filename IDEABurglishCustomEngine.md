# Introduction #

It's now clear that Burglish will have to be a C++ module in WaitZar. But how should we do this?

**Note:** Instead of just jumping in with the code, I want to practice documenting my thought process before coding. After writing this, I found that my overall way forward was clearer. Moreover, if another developer were to read this, she would know which classes to avoid editing for the next few days while I add this new feature.


# Step 1: Extend Input Manager or Model #

We have two sensible options:
  * Extend either InputMethod or RomanInputMethod
  * Extend WordBuilder

Given the first option:
  * Extending InputMethod itself makes no sense; we'd have to copy a lot of functionality from RomanInputMethod.
  * However, extending RomanInputMethod would require re-writing some simple stuff just to, e.g., clear a sentence. We could factor all this stuff out into functions, but then we'd have THREE levels of abstraction between typing and displaying.

Given the second option:
  * This would integrate seamlessly with the SentenceList.
  * Most of the functions map easily to what we'd use anyway (e.g., "getWords") and IDs can certainly be generated for Burglish words on an _ad hoc_ basis. Some functions like addRomanization() make little sense here.
  * In Java, we'd just make all the "useful" functions virtual and derive from there. But I don't want to slow down WaitZar w/ virtual function calls.


# Step 2: How to "Extend" WordBuilder #

I think templates might work:
  * Make InputMethod a templated class, and pass either a WordBuilder or a BurglishBuilder as a template param.
  * This would complicate error messages, but WordBuilder is stable enough for that now.
  * Performance would only take a hit in compiling; there's no runtime overhead for this method (except a tiny amount of memory).
  * Un-needed functions (i.e., those not called by InputManagers) could simply be excluded. This is a fairly clean way of doing it.


# Conclusions #

I will copy out the needed functions from WordBuilder into a BurglishBuilder header to test the general feasibility of this approach. Then, I will implement BurglishBuilder. Then, I will templatize the InputManager classes.