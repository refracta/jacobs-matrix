/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        display.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	Draws a view into a map.  Handles timed effects, map memory, etc.
 */

#ifndef __display__
#define __display__

#include "map.h"

#include "event.h"

class SCRPOS;

class DISPLAY
{
public:
    // Rectangle inside of TCOD to display.
    DISPLAY(int x, int y, int w, int h);
    ~DISPLAY();

    void	 display(POS pos, bool isblind=false);

    // This is in absolute screen coords, ie, bakes in myX and myY
    POS		 lookup(int x, int y) const;

    // Erases our memory.
    void	 clear();

    int		 width() const { return myW; }
    int		 height() const { return myH; }
    int		 x() const { return myX; }
    int		 y() const { return myY; }

    EVENT_QUEUE	 &queue() { return myEQ; }

private:
    void	 scrollMemory(int dx, int dy);
    void	 rotateMemory(int angle);
    u8		 recall(int x, int y) const;
    void	 note(int x, int y, u8 val);

    u8		*myMemory;
    int		 myX, myY;
    int		 myW, myH;
    int		 myMapId;
    SCRPOS	*myPosCache;

    // Where we track all of our active events.
    EVENTLIST	 myEvents;

    // Where incoming events go
    EVENT_QUEUE	 myEQ;
};

#endif
