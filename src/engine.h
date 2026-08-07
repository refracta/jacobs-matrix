/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        engine.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	Our game engine.  Grabs commands from its command
 *	queue and processes them as fast as possible.  Provides
 *	a mutexed method to get a recent map from other threads.
 *	Note that this will run on its own thread!
 */

#ifndef __engine__
#define __engine__

#include "command.h"
#include "thread.h"

class THREAD;
class MAP;
class DISPLAY;
class MOB;

class ENGINE
{
public:
    ENGINE(DISPLAY *display, MOB *avatar);
    ~ENGINE();

    // These methods are thread safe.
    CMD_QUEUE		&queue() { return myQueue; }
    // Returns a map with the reference copy incremented, you must
    // decRef when done!
    MAP			*copyMap();

    // Public only for call back conveninece.
    void		 mainLoop();

protected:
    /// Called when we are happy with myMap and want to publish it.
    void		 updateMap();

private:
    MAP			*myMap;
    MAP			*myOldMap;
    LOCK		 myMapLock;
    CMD_QUEUE		 myQueue;
    DISPLAY		*myDisplay;

    THREAD		*myThread;
    MOB			*myInitAvatar;
};

#endif
