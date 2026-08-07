Welcome to Jacob's Matrix!

===========================
  ABOUT
===========================

This is a Seven Day Roguelike.  It was written in a 168 hour time
frame.  Hopefully it doesn't feel like that was the case, however.

Jacob's Matrix is written by Jeff Lait and uses The Doryen Library
for graphical output.  

The music is Duegar Requiem by quasist, which can be found at the Rogue
Bard, http://roguebard.eptalys.net/song.php?id=23

You can contact Jeff Lait at: jmlait [snail] zincland [dot] com.

===========================
  HOW TO RUN
===========================

If you are a windows users, run it from windows/jacob.exe.

If you are a source diver, go into the src directory.
Look for BUILD.TXT files to try and guide you, there is one under
each of the porting subdirectories.

===========================
  IT IS TOO SLOW
===========================

A good amount of brute force and ignorance went into the fire effects.
You can edit the jacob.cfg file to reduce their quality.  Width
20, height 50 will be four times faster without too much quality
change.  Alternatively, set the width to 0 to disable flames entirely,
probably a good idea if you don't have a multi-core box.

===========================
  HOW TO PLAY
===========================

Movement is done with the number pad or with vi style keys.  You 
can also use the cursor keys for four way movement.
    7 8 9  y k u
     \|/    \|/
    4-5-6  h-.-l
     /|\    /|\
    1 2 3  b j n

 Command Keys:
   f,0 - Cast Spell
   e,/ - Rotate Map Left
   r,* - Rotate Map Right
   t,- - Orange Portal
   g,+ - Blue Portal
   Q - Quit
   D - Drop Weapon and Spellbook
   R - Restart Level
   x - Look Around
   w - Change Behaviour
   v - Declare Victory
   z - Welcome Message
   a - About Jacob's Matrix
   P - Toggle Fullscreen
   ? - This Help
     - Wait

===========================
  CUSTOMIZING 
===========================

If you don't like the default font, you can change it!

Go to the windows directory and replace terminal.png with another font
compatible with The Doryen Library.

If you don't like the music, you can edit jacob.cfg to point to
another file of your choosing.  You can also disable the music, this
will also let you cheat as you won't die when the song ends as it
never ends.

===========================
  ARTISITC MESSAGE
===========================

No attempt at deeper meaning, merely an attempt at a fun game.

I have always wanted to make a roguelike where the individual cells
don't have to be connected in a regular grid.  The most obvious
advantage is that when building a map you can just stitch rooms
together without having to worry about getting their spacing right.
Another neat advantage is you can borrow Portal's gameplay mechanics.

The second aspect of this game drew from my attempt to make a
roguelike around music.  Duegar Requiem had this feeling of sadness
and strangeness that seemed to fit my vision of a twisty turny
dungeon.  With the Doryen Library moving me into the real-time
mindset, I decided to make a roguelike which, while still turn based,
had a realtime component.  I decided to aim for a frantic paced
roguelike where you type as fast as you can to try and win.  This
meant keeping the interface as streamlined as possible - all the
essential controls are available on the number pad.

===========================
  FINAL IMPORTANT NOTE
===========================

Have fun!
