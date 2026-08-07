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

#ifndef __map__
#define __map__

#include <iostream>
using namespace std;

#include "ptrlist.h"
#include "thread.h"
#include "glbdef.h"

class POS;
class ROOM;
class MOB;
class ITEM;
class MAP;
class FRAGMENT;
class DISPLAY;
class SCRPOS;

typedef PTRLIST<ROOM *> ROOMLIST;
typedef PTRLIST<MOB *> MOBLIST;
typedef PTRLIST<ITEM *> ITEMLIST;
typedef PTRLIST<POS> POSLIST;
typedef PTRLIST<FRAGMENT *> FRAGMENTLIST;

// The position is the only way to access elements of the map
// because it can adjust for orientation, etc.
// Functions that modify the underlying square are const if they
// leeave this unchanged...
class POS
{
public:
    POS();
    // Required for ptrlist, sigh.
    explicit POS(int blah);

    /// Equality ignores angles.
    bool 	operator==(POS &cmp) const
    {	return cmp.myX == myX && cmp.myY == myY && cmp.myRoomId == myRoomId; }

    POS		left() const { return delta(-1, 0); }
    POS		up() const { return delta(0, -1); }
    POS		down() const { return delta(0, 1); }
    POS		right() const { return delta(1, 0); }

    int		angle() const { return myAngle; }

    POS		delta4Direction(int angle) const;
    POS		delta(int dx, int dy) const;

    POS		rotate(int angle) const;
    void	setAngle(int angle);

    bool	valid() const;

    /// Compute number of squares to get to goal, 8 way metric.
    /// This is an approximation, ignoring obstacles
    int		dist(POS goal) const;

    bool	victoryPos() const;

    /// Returns an estimate of what should be delta()d to this
    /// to move closer to goal.  Again, an estimate!  Traditionally
    /// just SIGN.
    void	dirTo(POS goal, int &dx, int &dy) const;

    /// Returns the first mob along the given vector from here.
    MOB		*traceBullet(int range, int dx, int dy, int *rangeleft=0) const;

    /// Returns the last valid pos before we hit a wall, if stop
    /// before wall set.  Otherwise returns the wall hit.
    POS		 traceBulletPos(int range, int dx, int dy, bool stopbeforewall, bool stopatmob = true) const;

    void	 fireball(MOB *caster, int rad, DPDF dpdf, u8 sym, ATTR_NAMES attr) const;

    /// Dumps to message description fo this square
    void	 describeSquare(bool blind) const;

    /// Draws the given bullet to the screen.
    void	 displayBullet(int range, int dx, int dy, u8 sym, ATTR_NAMES attr, bool stopatmob) const;

    void	 postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr) const;

    /// Returns current distance map here.
    int		 getDistance() const;
    void	 setDistance(int dist) const;

    TILE_NAMES	tile() const;
    void	setTile(TILE_NAMES tile) const;
    MAPFLAG_NAMES		flag() const;
    void	setFlag(MAPFLAG_NAMES flag, bool state) const;

    /// prepares a square to be floored, ie, wall all surrounding
    /// and verify no portals.  Returns false if fails, note
    /// some walls may be created.
    bool	prepSquareForDestruction() const;

    /// Digs out this square
    /// Returns false if undiggable.  Might also fail if adjacent
    /// true squares aren't safe as we don't want to dig around portals,
    /// etc
    bool	digSquare() const;

    bool	isFOV() const { return flag() & MAPFLAG_FOV ? true : false; }
    bool	isFOVCache() const { return flag() & MAPFLAG_FOVCACHE ? true : false; }

    MOB 	*mob() const;
    ITEM 	*item() const;

    MAP		*map() const { return myMap; }

    // Query functoins
    const TILE_DEF &defn() const { return defn(tile()); }
    static const TILE_DEF &defn(TILE_NAMES tile) { return glb_tiledefs[tile]; }

    bool	 isPassable() const { return defn().ispassable; }

    void	 setMap(MAP *map) { myMap = map; if (!map) myRoomId = -1; }

    void	 save(ostream &os) const;
    // In place load
    void	 load(istream &is);

    // A strange definition of const..
    void	 removeMob(MOB *mob) const;
    void	 addMob(MOB *mob) const;
    void	 removeItem(ITEM *item) const;
    void	 addItem(ITEM *item) const;

    int 	 getAllMobs(MOBLIST &list) const;
    int 	 getAllItems(ITEMLIST &list) const;

    // Returns colour of the room we are in.  Do not stash this
    // pointer.
    u8		*color() const;
    int		*jacobian() const;

    // Use with much discretion!
    int		 roomId() const { return myRoomId; }


private:
    ROOM	*room() const;
    // Transforms dx & dy into our local facing.
    void	 rotateToLocal(int &dx, int &dy) const;
    // And back to world.
    void	 rotateToWorld(int &dx, int &dy) const;

    // Null if not on a map.
    int		 myRoomId;
    MAP		*myMap;

    // The room relative coords
    int			 myX, myY;
    // Which way we face in the room.
    int			 myAngle;

    friend class ROOM;
    friend class FRAGMENT;
    friend class MAP;
};

/// Note that portals are directed.  mySrc is assuemd to have angle of 0
/// as the turning effect is stored in myDst's angle.
class PORTAL
{
public:
    POS		mySrc, myDst;
};

class ROOM
{
    ROOM();
    ROOM(const ROOM &room);
    ROOM &operator=(const ROOM &room);

    ~ROOM();

    int		width() const { return myWidth; }
    int		height() const { return myHeight; }

    static bool link(ROOM *a, int dira, ROOM *b, int dirb);
    static bool buildPortal(POS a, int dira, POS b, int dirb, bool settile=true);

    // mySrc is the portal square to launch from, myDst is the
    // destination square to land on
    POS		findProtoPortal(int dir);

private:
    // These all work in the room's local coordinates.
    TILE_NAMES		getTile(int x, int y) const;
    void		setTile(int x, int y, TILE_NAMES tile);
    MAPFLAG_NAMES	getFlag(int x, int y) const;

    void		setFlag(int x, int y, MAPFLAG_NAMES flag, bool state);
    void		setAllFlags(MAPFLAG_NAMES flag, bool state);

    int			getDistance(int x, int y) const;
    void		setDistance(int x, int y, int dist);

    void		setColor(u8 r, u8 g, u8 b) 
			{ myColor[0] = r; myColor[1] = g; myColor[2] = b; }

    MOB			*getMob(int x, int y) const;
    void		 addMob(int x, int y, MOB *mob);
    void		 removeMob(MOB *mob);
    ITEM		*getItem(int x, int y) const;
    void		 addItem(int x, int y, ITEM *item);
    void		 removeItem(ITEM *item);
    PORTAL		*getPortal(int x, int y) const;

    // Remvoes the portal that lands at dst.
    void		 removePortal(POS dst);

    void	 	 getAllMobs(int x, int y, MOBLIST &list) const;
    void	 	 getAllItems(int x, int y, ITEMLIST &list) const;

    void		 setMap(MAP *map);

    void		 resize(int w, int h);

    POS			 buildPos(int x, int y);
    POS			 getRandomPos(POS p, int *n);

    void		deleteContents();

    int			myId;
    MOBLIST		myMobs;
    ITEMLIST		myItems;
    PTRLIST<PORTAL>	myPortals;

    int			myWidth, myHeight;
    u8			*myTiles;
    u8			*myFlags;
    int			*myDists;
    MAP			*myMap;

    int			 myJacob[4];
    u8			 myColor[3];

    const FRAGMENT	*myFragment;

    friend class POS;
    friend class FRAGMENT;
    friend class MAP;
};

class MAP
{
public:
    explicit MAP(int depth, MOB *avatar, DISPLAY *display);
    MAP(const MAP &map);
    MAP &operator=(const MAP &map);

    void			incRef();
    void			decRef();

    MOB				*avatar() const { return myAvatar; }
    void			 setAvatar(MOB *mob) { myAvatar = mob; }

    void			 doMoveNPC();

    int				 getNumMobs() const;

    void			 setAllFlags(MAPFLAG_NAMES flag, bool state);

    void			 buildDistMap(POS center);

    int				 getId() const { return myUniqueId; }
    int				 getDepth() const { return myDepth; }

    static void			 init();

    // Computes the delta to add to a to get to b.
    // Returns false if unable to compute it.  Things outside of FOV
    // are flakey as we don't cache them as I'm very, very, lazy.
    // And I already burned all the free cycles in the fire sim.
    bool			 computeDelta(POS a, POS b, int &dx, int &dy);

    // Reinitailizes FOV around avatar.
    void			 rebuildFOV();

    void			 buildUserPortal(POS pos, int pnum, int dir);

protected:
    ~MAP();

    void			deleteContents();

    MOB				*findAvatar();

    ATOMIC_INT32		myRefCnt;

    int				myUniqueId;
    int				myDepth;

    POS				myUserPortal[2];
    int				myUserPortalDir[2];

    ROOMLIST			myRooms;

    MOB				*myAvatar;

    // The map as seen from our FOV.
    SCRPOS			*myFOVCache;

    DISPLAY			*myDisplay;

    static FRAGMENTLIST		 theFragments;

    friend class FRAGMENT;
    friend class ROOM;
    friend class POS;
};


#endif
