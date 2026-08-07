/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        event.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	The event queue sends display info back to the display.
 */

#ifndef __event__
#define __event__

#include "glbdef.h"
#include "queue.h"

class EVENT
{
public:
    EVENT()
    {
	mySym = '!';
	myAttr = ATTR_PURPLE;
	myFirstTS = -1;
	myType = EVENTTYPE_NONE;
    }
    explicit EVENT(int blah)
    {
	mySym = '!';
	myAttr = ATTR_PURPLE;
	myFirstTS = -1;
	myType = EVENTTYPE_NONE;
    }
    EVENT(POS p, u8 sym, ATTR_NAMES a, EVENTTYPE_NAMES type)
    {
	myPos = p;
	mySym = sym;
	myAttr = a;
	myFirstTS = -1;
	myType = type;
    }

    POS		pos() const { return myPos; }
    u8		sym() const { return mySym; }
    ATTR_NAMES	attr() const { return myAttr; }
    int		timestamp() const { return myFirstTS; }
    void	setTime(int ts) { myFirstTS = ts; }
    EVENTTYPE_NAMES type() const { return myType; }

private:
    u8		mySym;
    ATTR_NAMES	myAttr;
    POS		myPos;
    int		myFirstTS;	// Timestamp we proced.
    EVENTTYPE_NAMES	myType;
};

typedef QUEUE<EVENT> EVENT_QUEUE;
typedef PTRLIST<EVENT> EVENTLIST;

#endif

