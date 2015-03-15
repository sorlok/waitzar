# Goals #

As a romanisation attempt, WaitZar's one goal is simplicity. There are many ways WaitZar expresses this simplicity:
  * The source project is simple, and all third-party libraries are included internally.
  * The trunk source (for Windows) uses Win32 and C/C++, and can run -as a single .exe- off of a thumb drive without installing anything. It is intended for use on Internet Cafe computers. (The Linux version trades this portability for stability by running as a .deb package.)
  * A Myanmar word has only _one_ romanisation, governed by a few simple rules and shortcuts. Only lowercase QWERTY keys are used.
  * The license is short & succinct, to encourage others to branch off from this source with their own ideas. However, an API, a user interaction specification, and a testing guide are provided to help spinoffs to ensure they function just like normal WaitZar.
  * The model stores Myanmar text without using UTF-8, and can be loaded in any language that supports arrays. Additional words can be added through a UTF-8 file, in a format simple for users..


# Usage #

To use WaitZar, you should have at least one of the following fonts installed on your system:
  * Myanmar3
  * Padauk
  * Parabaik
Win Innwa and Zawgyi-One are also supported, if you prefer these, but we encourage you to use Unicode 5.1 when possible. You don't actually need _any_ font installed (but you won't see numbers/letters properly after you type them).

Simply run WaitZar.exe from any location. A white exclamation point ! will appear in your system's tray. Wait for this to change to the letters "ENG". You are now typing in English; you shouldn't notice a difference.

To switch to Burmese,
> hold **Ctrl** and hit **Shift**   --note that this also switches _back_ to English if pressed again.
...and "mya" will be displayed on the tray. Open Notepad. Now, type "waitzar". When you type the first "w", a box will pop up showing your progress. You can drag this to any comfortable location on the screen. After typing the final "r", hit the Spacebar. Now, the word (ဝိဇၨာ) will appear in a "sentence window". Press "5" and the number "၅" will also appear in the sentence window. Press left and right to move the cursor, and continue typing more words. When you are satisfied, press "Enter" and the full sentence will appear in Notepad. (You can also press "," or "." to finish the sentence and add a half-stop or full-stop respectively. If you see several boxes, you will have to change the font to display Myanmar3. (Note: In Open Office, you might not see anything at all! Hit Ctrl+A to select all the text, and change it to Myanmar3.)

To change the font,
> right-click on the "mya" icon, select "Encoding", and choose whatever encoding you prefer. Unicode 5.1 is the preferred (and default) encoding; you may also select Zawgyi-One or Win Innwa. Note: you need to be using **Wait Zar 1.5+ beta fix** or higher to select the output encoding.

To close the program,
> right-click on the "ENG" icon and choose "Exit". This is necessary if you need Alt+Shift to do some other task (such as switch to Chinese).

Advanced Usage:
> Please see the Wait Zar user's guide for more detailed documentation.


# Get Involved #

The WaitZar project is just getting started! Click on the "issues" tab and post some suggestions or problems. We'd love to hear from you.