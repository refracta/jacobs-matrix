/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        mob.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __mob__
#define __mob__

#include "glbdef.h"

#include "grammar.h"
#include "dpdf.h"
#include "ptrlist.h"
#include "map.h"

#include <iostream>
using namespace std;

class MAP;
class ITEM;

enum AI_STATE
{
    AI_STATE_EXPLORE,		// The default.
    AI_STATE_NEXTLEVEL,
    AI_STATE_HUNT,
    AI_STATE_PACKRAT,
};

class MOB
{
public:
    static MOB		*create(MOB_NAMES def);

    static MOB		*createNPC(int depth);

    // Creates a copy.  Ideally we make this copy on write so
    // is cheap, but mobs probably move every frame anyways.
    MOB			*copy() const;
    
    ~MOB();

    const POS		&pos() const { return myPos; }
    void		 setPos(POS pos);
    void		 setMap(MAP *map);
    void		 clearAllPos();

    bool		 alive() const { return myHP > 0; }

    int			 getHP() const { return myHP; }
    int			 getMP() const { return myMP; }

    void		 gainHP(int hp);
    void		 gainMP(int mp);

    int			 numDeaths() const { return myNumDeaths; }

    void		 setHome(POS pos) { myHome = pos; }

    bool		 isSwallowed() const { return myIsSwallowed; }
    void		 setSwallowed(bool isswallowed) { myIsSwallowed = isswallowed; }

    static void		 setAvatarRole(ROLE_NAMES role)
			 { theAvatarRole = role; }
    static ROLE_NAMES	 avatarRole() { return theAvatarRole; }

    // Returns true if we die!
    bool		 applyDamage(MOB *src, int hp);

    MOB_NAMES		 getDefinition() const { return myDefinition; }

    const MOB_DEF	&defn() const { return defn(getDefinition()); }
    static const MOB_DEF &defn(MOB_NAMES mob) { return glb_mobdefs[mob]; }

    bool		 isAvatar() const { return this == getAvatar(); }
    MOB			*getAvatar() const { if (pos().map()) return pos().map()->avatar(); return 0; }

    VERB_PERSON		 getPerson() const;

    // Symbol and attributes for rendering.
    void		 getLook(u8 &symbol, ATTR_NAMES &attr) const;

    // Retrieve the raw name.
    const char		*getName() const;
    const char		*getRawName() const;

    // Returns true if we have the item in question.
    bool		 hasItem(ITEM_NAMES itemname) const;
    ITEM 		*lookupItem(ITEM_NAMES itemname) const;
    ITEM		*getRandomItem() const;

    int			 getMeleeDamage() const;
    const char		*getMeleeVerb() const;
    DPDF		 getMeleeDPDF() const;
    int			 getRangedDamage() const;
    DPDF		 getRangedDPDF() const;
    int			 getRangedRange() const;

    void		 getRangedLook(u8 &symbol, ATTR_NAMES &attr) const;

    bool		 canMoveDir(int dx, int dy, bool checkmob = true) const;
    bool		 canMove(POS pos, bool checkmob = true) const;

    void		 move(POS newpos, bool ignoreangle = false);

    bool		 isFriends(MOB *other) const;
    void		 viewDescription() const;

    void		 addItem(ITEM *item);
    void		 removeItem(ITEM *item, bool quiet = false);

    void		 clearBoredom() { myBoredom = 0; }

    //
    // High level AI functions.
    //

    AI_NAMES		 getAI() const;

    // Determines if there are any mandatory actions to be done.
    // Return true if a forced action occured, in which case the
    // player gets no further input.
    bool		 aiForcedAction();

    // Runs the normal AI routines.  Called if aiForcedAction failed
    // and not the avatar.
    bool		 aiDoAI();

    // Updates our lock on the avatar - tracking if we know
    // where he is.  Position of last known location is in myTX.
    bool		 aiAcquireAvatar();
    bool		 aiAcquireTarget(MOB *foe);

    // Determine which item we like more.
    ITEM		*aiLikeMore(ITEM *a, ITEM *b) const;

    // Hugs the right hand walls.
    bool		 aiDoMouse();

    // Runs in straight lines until it hits something.
    bool		 aiStraightLine();

    // Charges the listed mob if in FOV
    bool		 aiCharge(MOB *foe, AI_NAMES aitype, bool orthoonly = false);

    // Can find mob anywhere on the mob - charges and kills.
    bool		 aiKillPathTo(MOB *target);

    // Attempts a ranged attack against the avatar
    bool		 aiRangeAttack(MOB *target = 0);

    // Runs away from (x, y).  Return true if action taken.
    bool		 aiFleeFrom(POS goal);
    bool		 aiFleeFromAvatar();

    // Runs straight towards (x, y).  Return true if action taken.
    bool		 aiMoveTo(POS goal, bool orthoonly = false);

    // Does a path find to get to x/y.  Returns false if blocked
    // or already at x & y.
    bool		 aiPathFindTo(POS goal);

    // Tries to go to (x, y) by flanking them.
    bool		 aiFlankTo(POS goal);
    
    bool		 aiRandomWalk(bool orthoonly = false);

    // Action methods.  These are how the AI and user manipulates
    // mobs.
    // Return true if the action consumed a turn, else false.
    bool		 actionBump(int dx, int dy);
    bool		 actionRotate(int angle);
    bool		 actionChat(int dx, int dy);
    bool		 actionMelee(int dx, int dy);
    bool		 actionWalk(int dx, int dy);
    bool		 actionFire(int dx, int dy);
    bool		 actionPortalFire(int dx, int dy, int portal);
    bool		 actionPickup();
    bool		 actionDrop();

    bool		 actionClimb();

    void		 save(ostream &os) const;
    static MOB		*load(istream &is);

    void		 getVisibleEnemies(PTRLIST<MOB *> &list) const;
    bool		 hasVisibleEnemies() const;
    
protected:
    MOB();

    MOB_NAMES		 myDefinition;

    POS			 myPos;

    int			 myNumDeaths;

    ITEMLIST		 myInventory;

    // Current target
    POS			 myTarget;
    int			 myFleeCount;
    int			 myBoredom;

    bool		 myIsSwallowed;

    // State machine
    int			 myAIState;

    // My home spot.
    POS			 myHome;

    // Hitpoints
    int			 myHP;
    // Mana points
    int			 myMP;

    static ROLE_NAMES    theAvatarRole;
};

#endif

