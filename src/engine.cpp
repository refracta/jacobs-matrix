/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        engine.cpp ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	Our game engine.  Grabs commands from its command
 *	queue and processes them as fast as possible.  Provides
 *	a mutexed method to get a recent map from other threads.
 *	Note that this will run on its own thread!
 */

#include "engine.h"

#include "msg.h"
#include "rand.h"
#include "map.h"
#include "mob.h"
#include "speed.h"
#include <time.h>

static void *
threadstarter(void *vdata)
{
    ENGINE	*engine = (ENGINE *) vdata;

    engine->mainLoop();

    return 0;
}

ENGINE::ENGINE(DISPLAY *display, MOB *avatar)
{
    myMap = myOldMap = 0;
    myDisplay = display;

    myInitAvatar = avatar;

    myThread = THREAD::alloc();
    myThread->start(threadstarter, this);
}

ENGINE::~ENGINE()
{
    if (myMap)
	myMap->decRef();
    if (myOldMap)
	myOldMap->decRef();
}

MAP *
ENGINE::copyMap()
{
    AUTOLOCK	a(myMapLock);

    if (myOldMap)
	myOldMap->incRef();

    return myOldMap;
}

void
ENGINE::updateMap()
{
    {
	AUTOLOCK	a(myMapLock);

	if (myOldMap)
	    myOldMap->decRef();

	myOldMap = myMap;
    }
    if (myOldMap)
    {
	myMap = new MAP(*myOldMap);
	myMap->incRef();
    }
}

#define VERIFY_ALIVE() \
    if (!avatar->alive()) \
    {			\
	msg_report("Dead people can't move.  ");	\
	break;			\
    }

void
ENGINE::mainLoop()
{
    rand_setseed((long) time(0));

    COMMAND		cmd;
    MOB			*avatar;
    bool		timeused = false;
    bool		doheartbeat = true;

    while (1)
    {
	avatar = 0;
	if (myMap)
	    avatar = myMap->avatar();

	timeused = false;
	if (doheartbeat && avatar && avatar->aiForcedAction())
	{
	    cmd = COMMAND(ACTION_NONE);
	    timeused = true;
	}
	else
	{
	    if (doheartbeat)
		msg_newturn();

	    // Allow the other thread a chance to redraw.
	    // Possible we might want a delay here?
	    updateMap();
	    avatar = 0;
	    if (myMap)
		avatar = myMap->avatar();
	    cmd = queue().waitAndRemove();

	    doheartbeat = false;
	}

	switch (cmd.action())
	{
	    case ACTION_RESTART:
	    {
		if (!avatar)
		{
		    if (myInitAvatar)
		    {
			avatar = myInitAvatar;
			myInitAvatar = 0;
			avatar->gainMP(avatar->defn().max_mp);
			avatar->gainHP(avatar->defn().max_hp);
		    }
		    else
		    {
			avatar = MOB::create(MOB_AVATAR);
		    }
		}
		else
		{
		    avatar = avatar->copy();
		    avatar->setMap(0);
		    avatar->gainMP(avatar->defn().max_mp);
		    avatar->gainHP(avatar->defn().max_hp);
		}

		if (myMap)
		    myMap->decRef();

		myMap = new MAP(cmd.dx(), avatar, myDisplay);
		myMap->incRef();
		myMap->rebuildFOV();
		break;
	    }

	    case ACTION_REBOOTAVATAR:
	    {
		if (avatar)
		    avatar->gainHP(avatar->defn().max_hp);
		break;
	    }

	    case ACTION_BUMP:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionBump(cmd.dx(), cmd.dy());
		break;
	    }
	
	    case ACTION_DROP:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionDrop();
		break;
	    }
	
	    case ACTION_ROTATE:
	    {
		if (avatar)
		    timeused = avatar->actionRotate(cmd.dx());
		break;
	    }

	    case ACTION_FIRE:
	    {
		VERIFY_ALIVE()
		if (avatar)
		{
		    if (!cmd.dz())
			timeused = avatar->actionFire(cmd.dx(), cmd.dy());
		    else
			timeused = avatar->actionPortalFire(cmd.dx(), cmd.dy(), cmd.dz()-1);
		}
		break;
	    }

	    case ACTION_SUICIDE:
	    {
		if (avatar)
		{
		    if (avatar->alive())
		    {
			msg_report("Your time has run out!  ");
			// We want the flame to die.
			avatar->gainHP(-avatar->getHP());
		    }
		}
		break;
	    }
	    case ACTION_CHANGEROLE:
	    {
		VERIFY_ALIVE()
		if (avatar)
		{
		    msg_format("You start acting like %O.  ",
				0,
				glb_roledefs[cmd.dx()].name);
		    MOB::setAvatarRole((ROLE_NAMES) cmd.dx());
		}
		break;
	    }

	    case ACTION_MANAPULSE:
		if (avatar && avatar->alive())
		{
		    avatar->gainMP(cmd.dx());
		}
		break;
	}

	if (myMap && timeused)
	{
	    // We need to build the FOV for the monsters as they
	    // rely on the updated FOV to track, etc.
	    // Rebuild the FOV map
	    myMap->rebuildFOV();

	    // Update the world.
	    myMap->doMoveNPC();
	    spd_inctime();

	    // Rebuild the FOV map
	    myMap->rebuildFOV();

	    doheartbeat = true;
	}

	// Allow the other thread a chance to redraw.
	// Possible we might want a delay here?
	updateMap();
    }
}
