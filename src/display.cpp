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


#include <libtcod.hpp>
#include "display.h"
#include "scrpos.h"
#include "gfxengine.h"
#include "mob.h"
#include "item.h"

DISPLAY::DISPLAY(int x, int y, int w, int h)
{
    myX = x;
    myY = y;
    myW = w;
    myH = h;
    myMapId = -1;

    myMemory = new u8[myW * myH];
    myPosCache = 0;
    clear();
}

DISPLAY::~DISPLAY()
{
    delete myPosCache;
    delete [] myMemory;
}

void
DISPLAY::clear()
{
    memset(myMemory, ' ', myW * myH);
    delete myPosCache;

    myPosCache = 0;
}

POS
DISPLAY::lookup(int x, int y) const
{
    POS		p;

    x -= myX + myW/2;
    y -= myY + myH/2;

    if (myPosCache)
	p = myPosCache->lookup(x, y);

    return p;
}

void
DISPLAY::scrollMemory(int dx, int dy)
{
    u8		*newmem = new u8[myW * myH];
    int		x, y;

    memset(newmem, ' ', myW * myH);

    for (y = 0; y < myH; y++)
    {
	if (y + dy < 0 || y + dy >= myH)
	    continue;
	for (x = 0; x < myW; x++)
	{
	    if (x + dx < 0 || x + dx >= myW)
		continue;

	    newmem[x+dx + (y+dy)*myW] = myMemory[x + y*myW];
	}
    }
    delete [] myMemory;
    myMemory = newmem;
}

void
DISPLAY::rotateMemory(int angle)
{
    angle = angle & 3;
    if (!angle)
	return;

    u8		*newmem = new u8[myW * myH];
    int		x, y;
    int		offx, offy;

    offx = myW/2;
    offy = myH/2;

    memset(newmem, ' ', myW * myH);

    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    switch (angle)
	    {
		case 1:
		    newmem[x + y*myW] = recall(y-offy+offx, myW - x-offx+offy);
		    break;

		case 2:
		    newmem[x + y*myW] = recall(myW - x, myH - y);
		    break;

		case 3:
		    newmem[x + y*myW] = recall(myH - y-offy+offx, x-offx+offy);
		    break;

	    }
	}
    }
    delete [] myMemory;
    myMemory = newmem;
}

u8
DISPLAY::recall(int x, int y) const
{
    if (x < 0 || x >= myW) return ' ';
    if (y < 0 || y >= myH) return ' ';
    return myMemory[x + y * myW];
}

void
DISPLAY::note(int x, int y, u8 val)
{
    myMemory[x + y * myW] = val;
}

void
DISPLAY::display(POS pos, bool isblind)
{
    int		dx, dy;
    int		x, y;

    if (!pos.map() || pos.map()->getId() != myMapId)
    {
	clear();
	if (pos.map())
	    myMapId = pos.map()->getId();
	else
	    myMapId = -1;
    }

    if (myPosCache && myPosCache->find(pos, dx, dy))
    {
	POS		oldpos = myPosCache->lookup(dx, dy);

	// Make pos the center
	scrollMemory(-dx, -dy);
	// Now rotate by it.
	rotateMemory(oldpos.angle() - pos.angle());
    }

    delete myPosCache;
    myPosCache = new SCRPOS(pos, (myW+1)/2, (myH+1)/2);

    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    pos = myPosCache->lookup(x-myW/2, y-myH/2);
	    if (!pos.valid() || !pos.isFOV() || (isblind && !(x == myW/2 && y == myH/2)))
	    {
		gfx_printchar(x+myX, y+myY, recall(x, y), ATTR_OUTOFFOV);
	    }
	    else
	    {
		note(x, y, pos.defn().symbol);
		if (pos.mob())
		    gfx_printchar(x+myX, y+myY, pos.mob()->defn().symbol, 
				    (ATTR_NAMES) pos.mob()->defn().attr);
		else if (pos.item())
		{
		    gfx_printchar(x+myX, y+myY, pos.item()->defn().symbol, 
				    (ATTR_NAMES) pos.item()->defn().attr);
		    note(x, y, pos.item()->defn().symbol);
		}
		else
		{
		    if (pos.defn().roomcolor)
			gfx_printchar(x+myX, y+myY, pos.defn().symbol, 
				pos.color()[0], pos.color()[1], pos.color()[2]);
		    else
			gfx_printchar(x+myX, y+myY, pos.defn().symbol, 
					(ATTR_NAMES) pos.defn().attr);
		}
	    }
	}
    }

    // Apply events...
    int		timems = TCOD_sys_elapsed_milli();

    // First process new events.
    EVENT	e;

    while (queue().remove(e))
    {
	e.setTime(timems);
	myEvents.append(e);
    }

    int		i;

    for (i = 0; i < myEvents.entries(); i++)
    {
	e = myEvents(i);

	if (timems - e.timestamp() > 150)
	{
	    myEvents.removeAt(i);
	    i--;
	    continue;
	}

	if (!myPosCache->find(e.pos(), x, y))
	{
	    // No longer on screen.
	    myEvents.removeAt(i);
	    i--;
	    continue;
	}

	// Get the actual pos which we can use for FOV
	pos = myPosCache->lookup(x, y);
	if (pos.isFOV())
	{
	    int		px = x + myX + myW/2;
	    int		py = y + myY + myH/2;

	    // Draw the event.
	    if (e.type() & EVENTTYPE_FORE)
		TCODConsole::root->setFore(px, py, 
				    TCODColor(glb_attrdefs[e.attr()].fg_r,
					       glb_attrdefs[e.attr()].fg_g,
					       glb_attrdefs[e.attr()].fg_b));
	    if (e.type() & EVENTTYPE_BACK)
		TCODConsole::root->setBack(px, py, 
				    TCODColor(glb_attrdefs[e.attr()].bg_r,
					       glb_attrdefs[e.attr()].bg_g,
					       glb_attrdefs[e.attr()].bg_b));
	    if (e.type() & EVENTTYPE_SYM)
		TCODConsole::root->setChar(px, py, e.sym());
	}
    }
}
