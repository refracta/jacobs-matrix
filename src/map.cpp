/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        map.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#include <libtcod.hpp>

#include "map.h"

#include "mob.h"
#include "item.h"

#include "text.h"

#include "dircontrol.h"
#include "scrpos.h"
#include "display.h"
#include "msg.h"

#include <fstream>
using namespace std;

PTRLIST<class FRAGMENT *> MAP::theFragments;

class FRAGMENT
{
public:
    FRAGMENT(const char *fname);
    ~FRAGMENT();

    ROOM		*buildRoom(MAP *map, int depth) const;

protected:
    int		myW, myH;
    u8		*myTiles;
    BUF		myFName;
    friend class ROOM;
};

POS::POS()
{
    myRoomId = -1;
    myMap = 0;
    myX = myY = myAngle = 0;
}

POS::POS(int blah)
{
    myRoomId = -1;
    myMap = 0;
    myX = myY = myAngle = 0;
}

bool	
POS::valid() const 
{ 
    if (tile() == TILE_INVALID) return false; 
    return true; 
}

TILE_NAMES	
POS::tile() const 
{ 
    if (!room()) return TILE_INVALID; 
    return room()->getTile(myX, myY); 
}

void	
POS::setTile(TILE_NAMES tile) const
{ 
    if (!room()) return; 
    return room()->setTile(myX, myY, tile); 
}

MAPFLAG_NAMES 		
POS::flag() const 
{ 
    if (!room()) return MAPFLAG_NONE; 
    return room()->getFlag(myX, myY); 
}

void	
POS::setFlag(MAPFLAG_NAMES flag, bool state) const
{ 
    if (!room()) return; 
    return room()->setFlag(myX, myY, flag, state); 
}

bool
POS::prepSquareForDestruction() const
{
    ROOM		*r = room();
    int			dx, dy;

    if (!valid())
	return false;

    if (!r)
	return false;

    if (!myX || !myY || myX == r->width()-1 || myY == r->height()-1)
    {
	// Border tile, borders to nothing, so forbid dig
	return false;
    }

    FORALL_8DIR(dx, dy)
    {
	if (r->getTile(myX+dx, myY+dy) == TILE_INVALID)
	{
	    // Expand our walls.
	    r->setTile(myX+dx, myY+dy, TILE_WALL);
	}
	// Do not dig around portals!
	if (r->getFlag(myX+dx, myY+dy) & MAPFLAG_PORTAL)
	    return false;
    }

    return true;
}

bool
POS::digSquare() const
{
    if (!defn().isdiggable)
	return false;

    if (!prepSquareForDestruction())
	return false;

    TILE_NAMES		nt;

    nt = TILE_FLOOR;
    if (tile() == TILE_WALL)
	nt = TILE_BROKENWALL;

    setTile(nt);

    return true;
}

bool
POS::victoryPos() const
{
    const u8	*rgb = color();
    if (rgb[0] == 255 && rgb[1] == 255 && rgb[2] == 0)
	return true;

    return false;
}

MOB *
POS::mob() const 
{ 
    if (!room()) return 0; 
    return room()->getMob(myX, myY); 
}

int
POS::getAllMobs(MOBLIST &list) const
{
    if (!room()) return list.entries();

    room()->getAllMobs(myX, myY, list);
    return list.entries();
}

int
POS::getDistance() const 
{ 
    if (!room()) return -1; 
    if (!(flag() & MAPFLAG_DIST))
	return -1;

    return room()->getDistance(myX, myY); 
}

void
POS::setDistance(int dist) const 
{ 
    if (!room()) return; 

    setFlag(MAPFLAG_DIST, true);

    return room()->setDistance(myX, myY, dist); 
}

int
POS::getAllItems(ITEMLIST &list) const
{
    if (!room()) return list.entries();

    room()->getAllItems(myX, myY, list);
    return list.entries();
}

ITEM *
POS::item() const 
{ 
    if (!room()) return 0; 
    return room()->getItem(myX, myY); 
}

ROOM *
POS::room() const 
{ 
    if (myRoomId < 0) return 0; 
    return myMap->myRooms(myRoomId); 
}

u8 *
POS::color() const
{
    static u8 invalid[3] = { 255, 255, 255 };
    if (myRoomId < 0) return invalid;

    return myMap->myRooms(myRoomId)->myColor;
}

int *
POS::jacobian() const
{
    static int j[4] = { 0, 0, 0, 0 };
    int		*newj;
    if (myRoomId < 0) return j;

    newj = myMap->myRooms(myRoomId)->myJacob;
    memcpy(j, newj, sizeof(int) * 4);
    rotateToWorld(j[0], j[2]);
    rotateToWorld(j[1], j[3]);

    return j;
}

void
POS::removeMob(MOB *mob) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->removeMob(mob);
}

void
POS::addMob(MOB *mob) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->addMob(myX, myY, mob);
}

void
POS::removeItem(ITEM *item) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->removeItem(item);
}

void
POS::addItem(ITEM *item) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->addItem(myX, myY, item);
}

void
POS::save(ostream &os) const
{
    os.write((const char *)&myX, sizeof(int));
    os.write((const char *)&myY, sizeof(int));
    os.write((const char *)&myAngle, sizeof(int));
    os.write((const char *)&myRoomId, sizeof(int));
}

void
POS::load(istream &is)
{
    is.read((char *)&myX, sizeof(int));
    is.read((char *)&myY, sizeof(int));
    is.read((char *)&myAngle, sizeof(int));
    is.read((char *)&myRoomId, sizeof(int));
}

MOB *
POS::traceBullet(int range, int dx, int dy) const
{
    if (!dx && !dy)
	return mob();

    POS		next = *this;

    while (range > 0)
    {
	range--;
	next = next.delta(dx, dy);
	if (next.mob())
	    return next.mob();

	// Stop at a wall.
	if (!next.defn().ispassable)
	    return 0;
    }
    return 0;
}

POS
POS::traceBulletPos(int range, int dx, int dy, bool stopbeforewall, bool stopatmob) const
{
    if (!dx && !dy)
	return *this;

    POS		next = *this;
    POS		last = *this;

    while (range > 0)
    {
	range--;
	next = next.delta(dx, dy);

	// Stop at a mob.
	if (stopatmob && next.mob())
	    return next;

	if (!next.defn().ispassable)
	{
	    // Hit a wall.  Either return next or last.
	    if (stopbeforewall)
		return last;
	    return next;
	}
	last = next;
    }
    return next;
}

void
POS::fireball(MOB *caster, int rad, DPDF dpdf, u8 sym, ATTR_NAMES attr) const
{
    int		dx, dy;

    POS		target;

    for (dy = -rad+1; dy <= rad-1; dy++)
    {
	for (dx = -rad+1; dx <= rad-1; dx++)
	{
	    target = delta(dx, dy);
	    if (target.valid())
	    {
		target.postEvent(EVENTTYPE_FORESYM, sym, attr);

		// It is too hard to play if you harm yourself.
		if (target.mob() && target.mob() != caster)
		{
		    target.mob()->applyDamage(caster, dpdf.evaluate());
		}
	    }
	}
    }
}

void
POS::postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr) const
{
    map()->myDisplay->queue().append(EVENT(*this, sym, attr, type));
}

void
POS::displayBullet(int range, int dx, int dy, u8 sym, ATTR_NAMES attr) const
{
    if (!dx && !dy)
	return;

    POS		next = delta(dx, dy);

    while (range > 0)
    {
	if (!next.valid())
	    return;
	
	// Stop at walls.
	if (!next.isPassable())
	    return;

	// Stop at mobs
	if (next.mob())
	    return;

	next.postEvent(EVENTTYPE_FORESYM, sym, attr);

	range--;
	next = next.delta(dx, dy);
    }
}

void
POS::rotateToLocal(int &dx, int &dy) const
{
    int		odx = dx;
    int		ody = dy;
    switch (myAngle & 3)
    {
	case 0:			// ^
	    break;		
	case 1:			// ->
	    dx = -ody;
	    dy = odx;
	    break;
	case 2:			// V
	    dx = -odx;
	    dy = -ody;
	    break;
	case 3:			// <-
	    dx = ody;
	    dy = -odx;
	    break;
    }
}

void
POS::rotateToWorld(int &dx, int &dy) const
{
    int		odx = dx;
    int		ody = dy;
    switch (myAngle & 3)
    {
	case 0:			// ^
	    break;		
	case 1:			// ->
	    dx = ody;
	    dy = -odx;
	    break;
	case 2:			// V
	    dx = -odx;
	    dy = -ody;
	    break;
	case 3:			// <-
	    dx = -ody;
	    dy = odx;
	    break;
    }
}

int
POS::dist(POS goal) const
{
    if (!goal.valid() || !valid())
    {
	return room()->width() + room()->height();
    }
    if (goal.myRoomId == myRoomId)
    {
	return MAX(ABS(goal.myX-myX), ABS(goal.myY-myY));
    }

    // Check to see if we are in the map cache.
    int		dx, dy;
    if (map()->computeDelta(*this, goal, dx, dy))
    {
	// Yah, something useful.
	return MAX(ABS(dx), ABS(dy));
    }

    // Consider it infinitely far.  Very dangerous as monsters
    // will get stuck on portals :>
    return room()->width() + room()->height();
}

void
POS::dirTo(POS goal, int &dx, int &dy) const
{
    if (!goal.valid() || !valid())
    {
	// Arbitrary...
	dx = 0;
	dy = 1;
	rotateToWorld(dx, dy);
    }

    if (goal.myRoomId == myRoomId)
    {
	dx = SIGN(goal.myX - myX);
	dy = SIGN(goal.myY - myY);

	// Convert to world!
	rotateToWorld(dx, dy);
	return;
    }

    // Check to see if we are in the map cache.
    if (map()->computeDelta(*this, goal, dx, dy))
    {
	// Map is nice enough to give it in world coords.
	dx = SIGN(dx);
	dy = SIGN(dy);
	return;
    }

    // Umm.. Be arbitrary?
    rand_direction(dx, dy);
}

void
POS::setAngle(int angle)
{
    myAngle = angle & 3;
}

POS
POS::rotate(int angle) const
{
    POS		p;

    p = *this;
    p.myAngle += angle;
    p.myAngle &= 3;

    return p;
}

POS
POS::delta4Direction(int angle) const
{
    int		dx, dy;

    rand_getdirection(angle, dx, dy);

    return delta(dx, dy);
}

POS
POS::delta(int dx, int dy) const
{
    if (!valid())
	return *this;

    if (!dx && !dy)
	return *this;

    // We want to make a single step in +x or +y.
    // The problem is diagonals.  We want to step in both xy and yx orders
    // and take whichever is valid.  In most cases they will be equal...
    if (dx && dy)
    {
	POS	xfirst, yfirst;
	int	sdx = SIGN(dx);
	int	sdy = SIGN(dy);


	xfirst = delta(sdx, 0).delta(0, sdy);
	yfirst = delta(0, sdy).delta(sdx, 0);

	if (!xfirst.valid())
	{
	    // Must take yfirst
	    return yfirst.delta(dx - sdx, dy - sdy);
	}
	if (!yfirst.valid())
	{
	    // Must take xfirst.
	    return xfirst.delta(dx - sdx, dy - sdy);
	}

	// Both are valid.  In all likeliehoods, identical.
	// But if one is wall and one is not, we want the non-wall!
	if (!glb_tiledefs[xfirst.tile()].ispassable)
	    return yfirst.delta(dx - sdx, dy - sdy);

	// WLOG, use xfirst now.
	return xfirst.delta(dx - sdx, dy - sdy);
    }

    // We now have a simple case of a horizontal step
    rotateToLocal(dx, dy);

    int		sdx = SIGN(dx);
    int		sdy = SIGN(dy);

    PORTAL	*portal;

    portal = room()->getPortal(myX + sdx, myY + sdy);
    if (portal)
    {
	// Teleport!
	POS		dst = portal->myDst;

	// We should remain on the same map!
	assert(dst.map() == myMap);

	// Apply the portal's twist.
	dst.myAngle += myAngle;
	dst.myAngle &= 3;

	// Portals have no thickness so we re-apply our original delta!
	// Actually, I changed my mind since then, so it is rather important
	// we actually do dec dx/dy.
	dx -= sdx;
	dy -= sdy;
	rotateToWorld(dx, dy);
	return dst.delta(dx, dy);
    }

    // Find the world delta to apply to our next square.
    dx -= sdx;
    dy -= sdy;
    rotateToWorld(dx, dy);

    // Get our next square...
    POS		dst = *this;

    dst.myX += sdx;
    dst.myY += sdy;

    if (dst.myX < 0 || dst.myX >= dst.room()->width() ||
	dst.myY < 0 || dst.myY >= dst.room()->height())
    {
	// An invalid square!
	dst.myRoomId = -1;
	return dst;
    }

    return dst.delta(dx, dy);
}

void
POS::describeSquare(bool isblind) const
{
    BUF		 buf;
    buf.reference(defn().legend);
    if (!isblind)
    {
	msg_format("You see %O.", 0, buf);
	
	if (mob())
	    msg_format("You see %O.", 0, mob());
	if (item())
	    msg_format("You see %O.", 0, item());
    }
}

///
/// ROOM functions
///

ROOM::ROOM()
{
    myTiles = myFlags = 0;
    myDists = 0;
    myFragment = 0;
    myId = -1;
    myColor[0] = myColor[1] = myColor[2] = 64;

    memset(myJacob, 0, sizeof(int) * 4);
}

ROOM::ROOM(const ROOM &room)
{
    myTiles = myFlags = 0;
    myDists = 0;
    *this = room;
}

ROOM &
ROOM::operator=(const ROOM &room)
{
    if (this == &room)
	return *this;

    deleteContents();

    resize(room.myWidth, room.myHeight);

    memcpy(myTiles, room.myTiles, sizeof(u8) * myWidth * myHeight);
    memcpy(myFlags, room.myFlags, sizeof(u8) * myWidth * myHeight);
    memcpy(myDists, room.myDists, sizeof(int) * myWidth * myHeight);
    memcpy(myColor, room.myColor, sizeof(u8) * 3);
    memcpy(myJacob, room.myJacob, sizeof(int) * 2 * 2);

    int		i;
    for (i = 0; i < room.myMobs.entries(); i++)
    {
	myMobs.append(room.myMobs(i)->copy());
    }
    for (i = 0; i < room.myItems.entries(); i++)
    {
	myItems.append(room.myItems(i)->copy());
    }
    myPortals = room.myPortals;
    myFragment = room.myFragment;
    myId = room.myId;

    // This usually requires some one to explicity call setMap()
    myMap = 0;

    return *this;
}


ROOM::~ROOM()
{
    deleteContents();
}

void
ROOM::deleteContents()
{
    int		i;

    for (i = 0; i < myMobs.entries(); i++)
    {
	delete myMobs(i);
    }
    myMobs.clear();
    for (i = 0; i < myItems.entries(); i++)
    {
	delete myItems(i);
    }
    myItems.clear();

    myPortals.clear();

    delete [] myTiles;
    delete [] myFlags;
    myTiles = myFlags = 0;

    delete [] myDists;
    myDists = 0;
}

TILE_NAMES
ROOM::getTile(int x, int y) const
{
    if (x < 0 || x >= width() || y < 0 || y >= height())
	return TILE_INVALID;

    return (TILE_NAMES) myTiles[x + y * width()];
}

void
ROOM::setTile(int x, int y, TILE_NAMES tile)
{
    myTiles[x + y * width()] = tile;
}

MAPFLAG_NAMES
ROOM::getFlag(int x, int y) const
{
    return (MAPFLAG_NAMES) myFlags[x + y * width()];
}

void
ROOM::setFlag(int x, int y, MAPFLAG_NAMES flag, bool state)
{
    if (state)
    {
	myFlags[x + y*width()] |= flag;
    }
    else
    {
	myFlags[x + y*width()] &= ~flag;
    }
}

void
ROOM::setAllFlags(MAPFLAG_NAMES flag, bool state)
{
    int		x, y;

    for (y = 0; y < height(); y++)
	for (x = 0; x < width(); x++)
	    setFlag(x, y, flag, state);
}

int
ROOM::getDistance(int x, int y) const
{
    return myDists[x + y * width()];
}

void
ROOM::setDistance(int x, int y, int dist)
{
    myDists[x + y * width()] = dist;
}

void
ROOM::setMap(MAP *map)
{
    int			i;

    myMap = map;

    for (i = 0; i < myMobs.entries(); i++)
    {
	myMobs(i)->setMap(map);
    }
    for (i = 0; i < myItems.entries(); i++)
    {
	myItems(i)->setMap(map);
    }
    for (i = 0; i < myPortals.entries(); i++)
    {
	PORTAL	p = myPortals(i);
	p.mySrc.setMap(map);
	p.myDst.setMap(map);
	myPortals.set(i, p);
    }
}

void
ROOM::resize(int w, int h)
{
    delete [] myTiles;
    delete [] myFlags;
    delete [] myDists;

    myWidth = w;
    myHeight = h;

    myTiles = new u8[myWidth * myHeight];
    memset(myTiles, 0, myWidth * myHeight);
    myFlags = new u8[myWidth * myHeight];
    memset(myFlags, 0, myWidth * myHeight);
    myDists = new int[myWidth * myHeight];
}

POS
ROOM::buildPos(int x, int y)
{
    POS		p;

    p.myMap = myMap;
    p.myRoomId = myId;
    p.myX = x;
    p.myY = y;
    p.myAngle = 0;

    return p;
}

POS
ROOM::getRandomPos(POS p, int *n)
{
    POS		np;
    int		x, y;
    int		ln = 1;

    if (!n)
	n = &ln;

    np.myMap = myMap;
    np.myRoomId = myId;
    np.myAngle = rand_choice(4);

    for (y = 0; y < height(); y++)
    {
	for (x = 0; x < width(); x++)
	{
	    np.myX = x; np.myY = y;

	    if (np.isPassable() && !np.mob())
	    {
		if (!rand_choice(*n))
		{
		    p = np;
		}
		// I hate ++ on * :>
		*n += 1;
	    }
	}
    }

    return p;
}

MOB *
ROOM::getMob(int x, int y) const
{
    int			i;

    for (i = 0; i < myMobs.entries(); i++)
    {
	if (myMobs(i)->pos().myX == x && myMobs(i)->pos().myY == y)
	{
	    // Ignore swallowed mobs or we'd attack ourselves.
	    if (myMobs(i)->isSwallowed())
		continue;
	    return myMobs(i);
	}
    }

    return 0;
}

void
ROOM::getAllMobs(int x, int y, MOBLIST &list) const
{
    int			i;

    for (i = 0; i < myMobs.entries(); i++)
    {
	if (myMobs(i)->pos().myX == x && myMobs(i)->pos().myY == y)
	{
	    list.append(myMobs(i));
	}
    }
}

void
ROOM::addMob(int x, int y, MOB *mob)
{
    myMobs.append(mob);
}
    
void
ROOM::removeMob(MOB *mob)
{
    myMobs.removePtr(mob);
}
    

ITEM *
ROOM::getItem(int x, int y) const
{
    int			i;

    for (i = 0; i < myItems.entries(); i++)
    {
	if (myItems(i)->pos().myX == x && myItems(i)->pos().myY == y)
	{
	    return myItems(i);
	}
    }

    return 0;
}
    
void
ROOM::getAllItems(int x, int y, ITEMLIST &list) const
{
    int			i;

    for (i = 0; i < myItems.entries(); i++)
    {
	if (myItems(i)->pos().myX == x && myItems(i)->pos().myY == y)
	{
	    list.append(myItems(i));
	}
    }
}
    
void
ROOM::addItem(int x, int y, ITEM *item)
{
    myItems.append(item);
}
    
void
ROOM::removeItem(ITEM *item)
{
    myItems.removePtr(item);
}
    
PORTAL *
ROOM::getPortal(int x, int y) const
{
    int			i;

    for (i = 0; i < myPortals.entries(); i++)
    {
	if (myPortals(i).mySrc.myX == x && myPortals(i).mySrc.myY == y)
	{
	    return myPortals.rawptr(i);
	}
    }

    return 0;
}

void
ROOM::removePortal(POS dst)
{
    int			i;

    for (i = 0; i < myPortals.entries(); i++)
    {
	if (myPortals(i).myDst == dst)
	{
	    myPortals.removeAt(i);
	    i--;
	}
    }
}

POS
ROOM::findProtoPortal(int dir)
{
    int		dx, dy, x, y, nfound;
    POS	result;

    rand_getdirection(dir, dx, dy);

    nfound = 0;

    for (y = 0; y < height(); y++)
    {
	for (x = 0; x < width(); x++)
	{
	    if (getTile(x, y) == TILE_PROTOPORTAL)
	    {
		if (getTile(x+dx, y+dy) == TILE_INVALID &&
		    getTile(x-dx, y-dy) == TILE_FLOOR)
		{
		    nfound++;
		    if (!rand_choice(nfound))
		    {
			result = buildPos(x, y);
		    }
		}
	    }
	}
    }

    if (nfound)
    {
    }
    else
    {
	cerr << "Failed to locate on map " << myFragment->myFName.buffer() << endl;
    }
    return result;
}

bool
ROOM::buildPortal(POS a, int dira, POS b, int dirb, bool settiles)
{
    PORTAL		 patob, pbtoa;

    if (!a.valid() || !b.valid())
	return false;

    patob.mySrc = a.delta4Direction(dira);
    patob.myDst = b;

    pbtoa.mySrc = b.delta4Direction(dirb);
    pbtoa.myDst = a;

    if (settiles)
    {
	patob.mySrc.setTile(TILE_SOLIDWALL);
	pbtoa.mySrc.setTile(TILE_SOLIDWALL);
    }

    patob.mySrc.myAngle = 0;
    pbtoa.mySrc.myAngle = 0;

    if (settiles)
    {
	patob.myDst.setTile(TILE_FLOOR);
	patob.myDst.setFlag(MAPFLAG_PORTAL, true);
	pbtoa.myDst.setTile(TILE_FLOOR);
	patob.myDst.setFlag(MAPFLAG_PORTAL, true);
    }

    patob.myDst.myAngle = (dira - dirb + 2) & 3;
    pbtoa.myDst.myAngle = (dirb - dira + 2) & 3;

    a.room()->myPortals.append(patob);
    b.room()->myPortals.append(pbtoa);

    return true;
}



bool
ROOM::link(ROOM *a, int dira, ROOM *b, int dirb)
{
    POS		aseed, bseed;

    aseed = a->findProtoPortal(dira);
    bseed = b->findProtoPortal(dirb);

    if (!aseed.valid() || !bseed.valid())
	return false;

    ROOM::buildPortal(aseed, dira, bseed, dirb);

    POS		an, bn;

    // Grow our portal in both directions as far as it will go.
    an = aseed.delta4Direction(dira+1);
    bn = bseed.delta4Direction(dirb-1);
    while (an.tile() == TILE_PROTOPORTAL && bn.tile() == TILE_PROTOPORTAL)
    {
	ROOM::buildPortal(an, dira, bn, dirb);
	an = an.delta4Direction(dira+1);
	bn = bn.delta4Direction(dirb-1);
    }

    an = aseed.delta4Direction(dira-1);
    bn = bseed.delta4Direction(dirb+1);
    while (an.tile() == TILE_PROTOPORTAL && bn.tile() == TILE_PROTOPORTAL)
    {
	ROOM::buildPortal(an, dira, bn, dirb);
	an = an.delta4Direction(dira-1);
	bn = bn.delta4Direction(dirb+1);
    }

    return true;
}

///
/// FRAGMENT definition and functions
///


FRAGMENT::FRAGMENT(const char *fname)
{
    ifstream		is(fname);
    char		line[500];
    PTRLIST<char *>	l;
    int			i, j;

    myFName.strcpy(fname);

    while (is.getline(line, 500))
    {
	text_striplf(line);

	// Ignore blank lines...
	if (line[0])
	    l.append(strdup(line));
    }

    if (l.entries() < 1)
    {
	cerr << "Empty map fragment " << fname << endl;
	exit(-1);
    }

    // Find smallest non-whitespace square.
    int			minx, maxx, miny, maxy;

    for (miny = 0; miny < l.entries(); miny++)
    {
	if (text_hasnonws(l(miny)))
	    break;
    }

    for (maxy = l.entries(); maxy --> 0; )
    {
	if (text_hasnonws(l(maxy)))
	    break;
    }

    if (miny > maxy)
    {
	cerr << "Blank map fragment " << fname << endl;
	exit(-1);
    }

    minx = strlen(l(miny));
    maxx = 0;
    for (i = miny; i <= maxy; i++)
    {
	minx = MIN(minx, text_firstnonws(l(i)));
	maxx = MAX(maxx, text_lastnonws(l(i)));
    }

    // Joys of rectangles with inconsistent exclusivity!
    // pad everything by 1.
    myW = maxx - minx + 1 + 2;
    myH = maxy - miny + 1 + 2;

    myTiles = new u8 [myW * myH];

    memset(myTiles, ' ', myW * myH);

    for (i = 1; i < myH-1; i++)
    {
	if (strlen(l(i+miny-1)) < myW + minx - 2)
	{
	    cerr << "Short line in fragment " << fname << endl;
	    exit(-1);
	}
	for (j = 1; j < myW-1; j++)
	{
	    myTiles[i * myW + j] = l(i+miny-1)[j+minx-1];
	}
    }

    for (i = 0; i < l.entries(); i++)
	free(l(i));
}

FRAGMENT::~FRAGMENT()
{
    delete [] myTiles;
}

ROOM *
FRAGMENT::buildRoom(MAP *map, int depth) const
{
    ROOM	*room;
    int		 x, y;
    MOB		*mob;
    TILE_NAMES	 tile;

    room = new ROOM();

    room->resize(myW, myH);
    room->setMap(map);
    room->myFragment = this;

    room->myId = map->myRooms.entries();
    map->myRooms.append(room);
    
    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    tile = TILE_INVALID;
	    switch (myTiles[x + y * myW])
	    {
		case 'A':
		    mob = MOB::createNPC(depth);

		    mob->move(room->buildPos(x, y));
		    tile = TILE_FLOOR;
		    break;

		case '.':
		    tile = TILE_FLOOR;
		    break;

		case '#':
		    tile = TILE_WALL;
		    break;

		case '+':
		    tile = TILE_PROTOPORTAL;
		    break;

		case ' ':
		    tile = TILE_INVALID;
		    break;
	    }

	    room->setTile(x, y, tile);
	}
    }

    return room;
}



static ATOMIC_INT32 glbMapId;

///
/// MAP functions
///
MAP::MAP(int depth, MOB *avatar, DISPLAY *display)
{
    myRefCnt.set(0);

    myDepth = depth;

    myUniqueId = glbMapId.add(1);
    myFOVCache = 0;

    FRAGMENT		*frag;

    int			 i, nrooms = 2, j;

    int			 nw, nh, nx, ny, dx, dy;
    int			**linkorder;
    int			**linkroom;

    nw = 2 + depth;
    nh = 2 + depth;

    nrooms = nw * nh;

    linkorder = new int *[nrooms];
    linkroom = new int *[nrooms];
    for (i = 0; i < nrooms; i++)
    {
	frag = theFragments(rand_choice(theFragments.entries()));
	frag->buildRoom(this, depth);

	linkorder[i] = new int[4];
	for (j = 0; j < 4; j++)
	    linkorder[i][j] = j;

	rand_shuffle(linkorder[i], 4);

	linkroom[i] = new int[4];
	for (j = 0; j < 4; j++)
	    linkroom[i][j] = i;

    }

    for (ny = 0; ny < nh; ny++)
    {
	for (nx = 0; nx < nw; nx++)
	{
	    i = nx + ny * nw;
	    myRooms(i)->setColor((u8) ((ny / (float)(nh-1)) * 255),
				 (u8) ((nx / (float)(nw-1)) * 255),
				 (u8) (255 - 255*((nx+ny) / (float)(nw+nh-2))));
	    FORALL_4DIR(dx, dy)
	    {
		if (nx + dx < 0)
		    continue;
		if (ny + dy < 0)
		    continue;
		// link does both ways so only link once!
		if (dx > 0)
		    continue;
		if (dy > 0)
		    continue;

		j = (nx + dx) + (ny + dy) * nh;

		ROOM::link(myRooms(i), linkorder[i][lcl_angle],
			   myRooms(j), linkorder[j][(lcl_angle+2)&3]);
		linkroom[i][linkorder[i][lcl_angle]] = j;
		linkroom[j][linkorder[j][(lcl_angle+2)&3]] = i;
	    }
	}
    }

    // Bulid the jacobian.
    for (ny = 0; ny < nh; ny++)
    {
	for (nx = 0; nx < nw; nx++)
	{
	    // Get adjacent room colours
	    i = nx + ny * nw;

	    // Change 
	    myRooms(i)->myJacob[0] = myRooms(linkroom[i][1])->myColor[0] - myRooms(linkroom[i][3])->myColor[0];
	    myRooms(i)->myJacob[1] = myRooms(linkroom[i][1])->myColor[1] - myRooms(linkroom[i][3])->myColor[1];
	    myRooms(i)->myJacob[2] = myRooms(linkroom[i][0])->myColor[0] - myRooms(linkroom[i][2])->myColor[0];
	    myRooms(i)->myJacob[3] = myRooms(linkroom[i][0])->myColor[1] - myRooms(linkroom[i][2])->myColor[1];

	}
    }

    if (avatar)
    {
	myAvatar = avatar;
	avatar->move(myRooms(0)->getRandomPos(POS(), 0));
    }

    for (i = 0; i < nrooms; i++)
    {
	delete [] linkorder[i];
	delete [] linkroom[i];
    }
    delete [] linkorder;
    delete [] linkroom;

    myDisplay = display;
}

MAP::MAP(const MAP &map)
{
    myRefCnt.set(0);
    myFOVCache = 0;
    *this = map;
}

MAP &
MAP::operator=(const MAP &map)
{
    int		i;

    if (this == &map)
	return *this;

    deleteContents();

    for (i = 0; i < map.myRooms.entries(); i++)
    {
	myRooms.append(new ROOM(*map.myRooms(i)));
	myRooms(i)->setMap(this);
    }

    for (i = 0; i < 2; i++)
    {
	myUserPortal[i] = map.myUserPortal[i];
	myUserPortal[i].setMap(this);
	myUserPortalDir[i] = map.myUserPortalDir[i];
    }

    myAvatar = findAvatar();
    
    myUniqueId = map.getId();

    myDepth = map.getDepth();

    myDisplay = map.myDisplay;

    // We need to build our own FOV cache to ensure poses refer
    // to us.
    myFOVCache = 0;

    return *this;
}

void
MAP::deleteContents()
{
    int		i;

    for (i = 0; i < myRooms.entries(); i++)
    {
	delete myRooms(i);
    }
    myRooms.clear();

    delete myFOVCache;
    myFOVCache = 0;
}

MAP::~MAP()
{
    deleteContents();
}

void
MAP::incRef()
{
    myRefCnt.add(1);
}

void
MAP::decRef()
{
    int		nval;
    nval = myRefCnt.add(-1);

    if (nval <= 0)
	delete this;
}

int
MAP::getNumMobs() const
{
    int		i, total = 0;

    for (i = 0; i < myRooms.entries(); i++)
    {
	total += myRooms(i)->myMobs.entries();
    }

    return total;
}

void
MAP::setAllFlags(MAPFLAG_NAMES flag, bool state)
{
    int		i;

    for (i = 0; i < myRooms.entries(); i++)
    {
	myRooms(i)->setAllFlags(flag, state);
    }
}

void
MAP::buildDistMap(POS goal)
{
    POSLIST		stack;
    POS			p, np;
    int			dist, dx, dy;

    setAllFlags(MAPFLAG_DIST, false);
    goal.setDistance(0);

    stack.append(goal);
    while (stack.entries())
    {
	p = stack.removeFirst();

	dist = p.getDistance();
	dist++;

	FORALL_8DIR(dx, dy)
	{
	    np = p.delta(dx, dy);

	    if (!np.valid())
		continue;

	    if (!np.isPassable())
		continue;

	    if (np.getDistance() >= 0)
		continue;

	    np.setDistance(dist);

	    stack.append(np);
	}
    }
}

void
MAP::doMoveNPC()
{
    MOBLIST		allmobs;
    int			i;

    // Pregather in case mobs move through portals.
    for (i = 0; i < myRooms.entries(); i++)
    {
	allmobs.append(myRooms(i)->myMobs);
    }

    // The avatar may die during this process so can't test against him
    // in the inner loop.
    allmobs.removePtr(avatar());

    // TODO: A mob may kill another mob, causing this to die!
    for (i = 0; i < allmobs.entries(); i++)
    {
	if (!allmobs(i)->aiForcedAction())
	    allmobs(i)->aiDoAI();
    }
}

MOB *
MAP::findAvatar()
{
    int			i, j;

    // Pregather in case mobs move through portals.
    for (i = 0; i < myRooms.entries(); i++)
    {
	for (j = 0; j < myRooms(i)->myMobs.entries(); j++)
	{
	    if (myRooms(i)->myMobs(j)->getDefinition() == MOB_AVATAR)
		return myRooms(i)->myMobs(j);
	}
    }
    return 0;
}

void
MAP::init()
{
    DIRECTORY		dir;
    const char		*f;
    BUF			buf;

    dir.opendir("../rooms");

    while (f = dir.readdir())
    {
	buf.strcpy(f);
	if (buf.extensionMatch("map"))
	{
	    buf.sprintf("../rooms/%s", f);
	    theFragments.append(new FRAGMENT(buf.buffer()));
	}
    }

    if (!theFragments.entries())
    {
	cerr << "No .map files found in ../rooms" << endl;
	exit(-1);
    }
}

void
MAP::rebuildFOV()
{
    setAllFlags(MAPFLAG_FOV, false);
    setAllFlags(MAPFLAG_FOVCACHE, false);

    if (!avatar())
	return;

    delete myFOVCache;

    myFOVCache = new SCRPOS(avatar()->pos(), 40, 25);
    POS			p;

    TCODMap		tcodmap(81, 51);
    int			x, y;

    for (y = 0; y < 51; y++)
    {
	for (x = 0; x < 81; x++)
	{
	    p = myFOVCache->lookup(x-40, y - 25);
	    p.setFlag(MAPFLAG_FOVCACHE, true);
	    tcodmap.setProperties(x, y, p.defn().istransparent, p.isPassable());
	}
    }

    tcodmap.computeFov(40, 25);

    for (y = 0; y < 51; y++)
    {
	for (x = 0; x < 81; x++)
	{
	    p = myFOVCache->lookup(x-40, y - 25);
	    // We might see this square in more than one way.
	    // We don't want to mark ourselves invisible if
	    // there is someway that did see us.
	    // This will result in too large an FOV, but the "free" squares
	    // are those that you could reproduce from what you already 
	    // can see, so I hope to be benign?
	    if (tcodmap.isInFov(x, y))
		p.setFlag(MAPFLAG_FOV, true);
	}
    }
}

bool
MAP::computeDelta(POS a, POS b, int &dx, int &dy)
{
    int		x1, y1, x2, y2;

    if (!myFOVCache)
	return false;

    // This early exits the linear search in myFOVCache find.
    if (!a.isFOVCache() || !b.isFOVCache())
	return false;

    if (!myFOVCache->find(a, x1, y1))
	return false;
    if (!myFOVCache->find(b, x2, y2))
	return false;

    dx = x2 - x1;
    dy = y2 - y1;

    POS		ap, at;

    // These are all in the coordinates of whoever built the scrpos,
    // ie, scrpos(0,0)
    //
    // This is a very confusing situation.
    // I think we have:
    // a - our source pos
    // @ - center of the FOV, myFOVCache->lookup(0,0)
    // a' - our source pos after walking from @, ie myFOVCache->lookup(x1,y1)
    //
    // dx, dy are in world space of @.  We want world space of a.
    // call foo.W the toWorld and foo.L the toLocal.
    // First apply the twist that @ got going to a'.
    // a'.W(@.L())
    // Next apply the transform into a.
    // a.W(a'.L())
    // Next we see a' cancels, leaving
    // a.W(@.L())
    //
    // Okay, that is bogus.  It is obvious it can't work as it
    // does not depend on a', and a' is the only way to include
    // the portal twist invoked in the walk!  I think I have the
    // twist backwards, ie, @.W(a'.L()), which means the compaction
    // doesn't occur.

    //
    // Try again.
    // @.L is needed to get into map space.
    // Then apply twist of @ -> a'   This is in Local coords!
    // So we just apply a.W
    // a.W(@.L(a'.W(@.L())))

    // Hmm.. 
    // ap encodes how to go from @ based world coords into
    // local a room map coords.  Thus ap.L gets us right there,
    // with only an a.W needed.

    ap = myFOVCache->lookup(x1, y1);
    // at = myFOVCache->lookup(0, 0);

    ap.rotateToLocal(dx, dy);
    a.rotateToWorld(dx, dy);

    return true;
}

void
MAP::buildUserPortal(POS pos, int pnum, int dir)
{
    if (!pos.valid())
	return;

    if (myUserPortal[pnum].valid())
    {
	// We already have a portal, must clean it up.
	myUserPortal[pnum].setFlag(MAPFLAG_PORTAL, false);

	if (myUserPortal[0].valid() && myUserPortal[1].valid())
	{
	    // Both existing portals are wiped.
	    // Note the reversal of directins here!  This is because we
	    // have the coordinates of the landing zone and the portal
	    // only exists in the source room!
	    myUserPortal[1].room()->removePortal(myUserPortal[0]);
	    myUserPortal[0].room()->removePortal(myUserPortal[1]);
	}

	// Restore the wall, possibly trapping people :>
	myUserPortal[pnum].setTile(TILE_WALL);
	myUserPortal[pnum] = POS();
    }

    // Construct our new portal...
    pos.setTile(pnum ? TILE_ORANGEPORTAL : TILE_BLUEPORTAL);
    pos.setFlag(MAPFLAG_PORTAL, true);
    myUserPortal[pnum] = pos;
    myUserPortalDir[pnum] = dir;

    // Build the link
    if (myUserPortal[0].valid() && myUserPortal[1].valid())
    {
	ROOM::buildPortal(myUserPortal[0], myUserPortalDir[0],
			  myUserPortal[1], myUserPortalDir[1],
			    false);	
    }
}
