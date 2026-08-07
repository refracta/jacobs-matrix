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
You can edit the jacob.cfg file to reduce their quality.  Reduce the
flame quality in the options menu to speed things up.  Alternatively,
edit the jacob.cfg file and change the flame width and height.  A
width of 20, height of 50, will be four times faster without too much
quality change.  Alternatively, set the width to 0 to disable flames
entirely, probably a good idea if you don't have a multi-core box.

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
   m - Map
   x - Look Around
   w - Change Behaviour
   v - Declare Victory
   z - Welcome Message
   a - About Jacob's Matrix
   P - Toggle Fullscreen
   ? - This Help
   P - Toggle Fullscreen
   O - Options
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
meant keeping the interface as streamlined as possible.  Decisions
such as which weapon or spellbook to keep was replaced by just setting
the adventurer's preference rather than trying to judge the quality of
each choice.  Potions and equipment are all instantly consumed.  The
required keys were minimized and fit on the number pad.  The player
can then focus on navigation and crowd control.

The Jacobian ended up an important part of the theme of the game.
Because of the way the rooms were randomly stitched together, a simple
compass can't be used to point players in the right direction as it
could be that both moving left a room and moving right a room both
bring the player closer to the destination.  The colour of the room
can be seen as the vector-output of the function on the current room.
If we make this function locally orientable from the screen-space, we
can easily define the Jacobian which will tell the user how to get the
colours more "yellow", ie, maximize red and green.  The last sentence
is of no use to anyone who hasn't already figured out how the
Jacobian works.  Note that +Y is Down, as per standard screen space,
and contrary to usual mathematical convention.

===========================
  CREDITS
===========================

This section is not complete without mentioning quasist, whose music
forms the backbone of this game.

Thanks to Physics for reproting the yellow light bug.  Thanks to Martin Bays
for pointing out that shooting yourself was not properly handled and
providing valuable feedback in how to better reveal the jacobian and
polish the portal throwing code.

===========================
  FINAL IMPORTANT NOTE
===========================

Have fun!
