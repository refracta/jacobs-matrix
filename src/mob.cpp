/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        mob.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "mob.h"

#include "map.h"
#include "msg.h"
#include "text.h"
#include "speed.h"
#include "item.h"

#include <iostream>
using namespace std;

ROLE_NAMES MOB::theAvatarRole;

//
// MOB Implementation
//

MOB::MOB()
{
    myFleeCount = 0;
    myBoredom = 0;
    myAIState = 0;
    myHP = 0;
    myIsSwallowed = false;
    myNumDeaths = 0;
}

MOB::~MOB()
{
    int		i;

    if (myPos.map() && myPos.map()->avatar() == this)
	myPos.map()->setAvatar(0);

    myPos.removeMob(this);

    for (i = 0; i < myInventory.entries(); i++)
	delete myInventory(i);
}

MOB *
MOB::create(MOB_NAMES def)
{
    MOB		*mob;

    mob = new MOB();

    mob->myDefinition = def;

    mob->myHP = glb_mobdefs[def].max_hp;
    mob->myMP = glb_mobdefs[def].max_mp;

#if 0
    if (def == MOB_AVATAR)
    {
	ITEM		*i;
	i = ITEM::create(ITEM_SPELLBOOK, 5);
	mob->addItem(i);
	i = ITEM::create(ITEM_WEAPON, 5);
	mob->addItem(i);
    }
#endif

    return mob;
}

MOB *
MOB::copy() const
{
    MOB		*mob;

    mob = new MOB();
    
    *mob = *this;

    // Copy inventory one item at a time.
    // We are careful to maintain the same list order here so restoring
    // won't shuffle things unexpectadly
    int		 i;

    mob->myInventory.clear();
    for (i = 0; i < myInventory.entries(); i++)
    {
	mob->myInventory.append(myInventory(i)->copy());
    }
    
    return mob;
}

void
MOB::setPos(POS pos)
{
    myPos.removeMob(this);
    myPos = pos;
    myPos.addMob(this);
}


void
MOB::setMap(MAP *map)
{
    myPos.setMap(map);
    myTarget.setMap(map);
    myHome.setMap(map);
}

void
MOB::clearAllPos()
{
    myPos = POS();
    myTarget = POS();
    myHome = POS();
}

MOB *
MOB::createNPC(int depth)
{
    int		i;
    MOB_NAMES	mob = MOB_NONE;
    int		choice = 0;
    MOB		*m;

    // Given the letter choice, choose a mob that matches it appropriately.
    choice = 0;
    for (i = 0; i < NUM_MOBS; i++)
    {
	// Stuff with 0 depth is never created manually.
	if (!glb_mobdefs[i].depth)
	    continue;

	// Use the baseletter to bias the creation.
	if (glb_mobdefs[i].depth <= depth)
	{
	    if (rand_choice(choice + glb_mobdefs[i].rarity) < glb_mobdefs[i].rarity)
		mob = (MOB_NAMES) i;
	    choice += glb_mobdefs[i].rarity;
	}
    }

    // Testing..
    // mob = MOB_PYTHON;

    m = MOB::create(mob);

    if (!rand_choice(3))
    {
	ITEM		*i = ITEM::createRandom(depth);
	m->addItem(i);
    }

    return m;
}

void
MOB::getLook(u8 &symbol, ATTR_NAMES &attr) const
{
    symbol = glb_mobdefs[myDefinition].symbol;
    attr = (ATTR_NAMES) glb_mobdefs[myDefinition].attr;
}

const char *
MOB::getName() const
{
    if (isAvatar())
	return "you";
    return glb_mobdefs[myDefinition].name;
}

const char *
MOB::getRawName() const
{
    return glb_mobdefs[myDefinition].name;
}

ITEM *
MOB::getRandomItem() const
{
    ITEM	*result;
    int		choice = 0;

    result = 0;

    if (myInventory.entries())
    {
	choice = rand_choice(myInventory.entries());
	return myInventory(choice);
    }
    return result;
}

bool
MOB::hasItem(ITEM_NAMES itemname) const
{
    // Prohibit monsters from using anything.
    if (getDefinition() != MOB_AVATAR)
    {
	return false;
    }
    if (lookupItem(itemname))
	return true;

    return false;
}

ITEM *
MOB::lookupItem(ITEM_NAMES itemname) const
{
    int		i;

    // Prohibit monsters from using anything.
    if (getDefinition() != MOB_AVATAR)
    {
	return 0;
    }
    for (i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->getDefinition() == itemname)
	    return myInventory(i);
    }
    return 0;
}

bool
MOB::canMoveDir(int dx, int dy, bool checkmob) const
{
    POS		goal;

    goal = myPos.delta(dx, dy);

    return canMove(goal, checkmob);
}

bool
MOB::canMove(POS pos, bool checkmob) const
{
    if (!pos.valid())
	return false;

    if (!pos.isPassable())
    {
	if (pos.defn().isphaseable && defn().passwall)
	{
	    // Can move through it...
	}
	else if (pos.defn().isdiggable && defn().candig)
	{
	    // Could dig through it.
	}
	else
	{
	    // Failed...
	    return false;
	}
    }

    if (checkmob && pos.mob())
	return false;

    return true;
}

void
MOB::move(POS newpos, bool ignoreangle)
{
    // If we have swallowed something, move it too.
    if (!isSwallowed())
    {
	PTRLIST<MOB *>	moblist;
	int		i;

	pos().getAllMobs(moblist);
	for (i = 0; i < moblist.entries(); i++)
	{
	    if (moblist(i)->isSwallowed())
	    {
		moblist(i)->move(newpos, true);
	    }
	}
    }

    if (ignoreangle)
	newpos.setAngle(pos().angle());

    setPos(newpos);
}

void
MOB::gainHP(int hp)
{
    int		maxhp;

    maxhp = defn().max_hp;

    myHP += hp;
    if (myHP > maxhp)
	myHP = maxhp;
}

void
MOB::gainMP(int mp)
{
    int		maxmp;

    maxmp = defn().max_mp;

    myMP += mp;
    if (myMP > maxmp)
	myMP = maxmp;
}

bool
MOB::applyDamage(MOB *src, int hits)
{
    // Being hit isn't boring.
    clearBoredom();

    if (hasItem(ITEM_INVULNERABLE))
	return false;

    if (hits >= getHP())
    {
	// Ensure we are dead to alive()
	myHP = 0;
	myNumDeaths++;
	// Rather mean.
	myMP = 0;

	// Death!
	if (src)
	    msg_format("%S <kill> %O!", src, this);
	else
	    msg_format("%S <be> killed!", this);

	// If there is a source, and they are swallowed,
	// they are released.
	if (src && src->isSwallowed())
	{
	    src->setSwallowed(false);
	}
	
	// If we are the avatar, special stuff happens.
	if (isAvatar())
	{
	    // never update on this thread!
	    // msg_update();
	    // TODO: Pause something here?
	}
	else
	{
	    // Drop any stuff we have.
	    int i;
	    for (i = 0; i < myInventory.entries(); i++)
	    {
		myInventory(i)->move(pos());
	    }
	    myInventory.clear();
	}

	if (src && src->isAvatar())
	{
	    // Award our score.

	}

	// Flash the screen
	pos().postEvent(EVENTTYPE_FOREBACK, ' ', ATTR_INVULNERABLE);

	// Note that avatar doesn't actually die...
	if (!isAvatar())
	    delete this;
	else
	{
	    // Make sure we drop our blind attribute..
	    loseTempItems();
	}
	return true;
    }

    // Flash that they are hit.
    if (hits)
	pos().postEvent(EVENTTYPE_FOREBACK, ' ', ATTR_HEALTH);

    myHP -= hits;
    return false;
}

VERB_PERSON
MOB::getPerson() const
{
    if (isAvatar())
	return VERB_YOU;

    return VERB_IT;
}

bool
MOB::isFriends(MOB *other) const
{
    if (other == this)
	return true;
    
    if (isAvatar())
    {
	if (other->defn().isfriendly)
	    return true;
	else
	    return false;
    }

    // Only the avatar is evil!
    if (defn().isfriendly)
    {
	return true;
    }
    else
    {
	// Monsters hate the avtar!
	if (other->isAvatar())
	    return false;
    }
    return true;
}

void
MOB::viewDescription() const
{
    char	buf[100];
    int		min, q1, q2, q3, max;

    sprintf(buf, "Name: %s\n", getName());
    msg_report(buf);

    sprintf(buf, "Health: %d\n", getHP());
    msg_report(buf);

    getMeleeDPDF().getQuartile(min, q1, q2, q3, max);
    sprintf(buf, "Melee Weapon: %s (%d..%d..%d..%d..%d)\n",
		defn().melee_name,
		min, q1, q2, q3, max);
    msg_report(buf);

    if (defn().range_valid)
    {
	getRangedDPDF().getQuartile(min, q1, q2, q3, max);
	sprintf(buf, "Ranged Weapon: %s (%d..%d..%d..%d..%d), Range %d\n",
		    defn().range_name,
		    min, q1, q2, q3, max,
		    getRangedRange());
	msg_report(buf);
    }
    // Dump the text.txt if present.
    msg_report(text_lookup("mob", getRawName()));
}

AI_NAMES
MOB::getAI() const
{
    return (AI_NAMES) defn().ai;
}

bool
MOB::actionBump(int dx, int dy)
{
    MOB		*mob;
    POS		 t;

    // Stand in place.
    if (!dx && !dy)
	return true;

    t = pos().delta(dx, dy);

    // If we are swallowed, we must attack.
    if (isSwallowed())
	return actionMelee(dx, dy);
    
    mob = t.mob();
    if (mob)
    {
	// Either kill or chat!
	if (mob->isFriends(this))
	{
	    return actionChat(dx, dy);
	}
	else
	{
	    return actionMelee(dx, dy);
	}
    }

    // No mob, see if we can move that way.
    // We let actionWalk deal with unable to move notification.
    return actionWalk(dx, dy);
}

bool
MOB::actionDrop()
{
    // Drop any stuff we have.
    int i;
    bool	fail = true;
    for (i = 0; i < myInventory.entries(); i++)
    {
	// Ignore special case items..
	// We don't want to drop "blindness" :>
	if (myInventory(i)->defn().rarity == 0)
	    continue;
	msg_format("%S <drop> %O.", this, myInventory(i));
	fail = false;
	myInventory(i)->move(pos());
	myInventory.set(i, 0);
    }
    myInventory.collapse();

    if (fail)
	msg_format("%S <drop> nothing.", this);

    return true;
}

bool
MOB::actionRotate(int angle)
{
    myPos = myPos.rotate(angle);
    // This is always a free action.
    return false;
}

bool
MOB::actionClimb()
{
    TILE_NAMES		tile;

    tile = pos().tile();

    if (tile == TILE_DOWNSTAIRS)
    {
	// Climbing is interesting.
	myFleeCount = 0;
	clearBoredom();
 
	msg_format("%S <climb> down... to nowhere!", this);
	return true;
    }
    else if (tile == TILE_UPSTAIRS)
    {
	msg_format("%S <climb> up... to nowhere!", this);
	return true;
    }
    else if (tile == TILE_FOREST)
    {
	msg_format("%S <climb> a tree... and <fall> down!", this);
	return true;
    }

    msg_format("%S <see> nothing to climb here.", this);

    return false;
}

bool
MOB::actionChat(int dx, int dy)
{
    MOB		*victim;

    // This should never occur, but to avoid
    // embarassaments...
    if (!isAvatar())
	return false;
    
    victim = pos().delta(dx, dy).mob();
    if (!victim)
    {
	// Talk to self
	msg_format("%S <talk> to %O!", this, this);
	return false;
    }

    msg_format("%S <chat> with %O:\n", this, victim);

    msg_quote(text_lookup("chat", getRawName(), victim->getRawName()));
    
    return true;
}

bool
MOB::actionMelee(int dx, int dy)
{
    MOB		*victim;

    // If we are swallowed, attack the person on our square.
    if (isSwallowed())
	dx = dy = 0;

    POS		 t = pos().delta(dx, dy);

    victim = t.mob();
    if (!victim)
    {
	// Swing in air!
	msg_format("%S <swing> at empty air!", this);
	return false;
    }

    // Shooting is interesting.
    clearBoredom();
 
    int		damage;
    bool	victimdead;

    damage = getMeleeDamage();

    // attempt to kill victim
    if (damage || (defn().melee_item != ITEM_NONE))
    {
	msg_format("%S %v %O.", this, getMeleeVerb(),
				victim);
    }
    else
    {
	msg_format("%S <miss> %O.", this, victim);
    }

    victimdead = victim->applyDamage(this, damage);
    
    // Vampires regenerate!
    if (defn().isvampire)
    {
	myHP += damage;
    }

    // Grant our item...
    if (!victimdead)
    {
	ITEM_NAMES	item;

	item = (ITEM_NAMES) defn().melee_item;
	if (item != ITEM_NONE)
	{
	    ITEM	*i;

	    // Exclusive items you should only get one of so they
	    // don't stack.
	    if (!glb_itemdefs[item].exclusive ||
		!victim->hasItem(item))
	    {
		i = ITEM::create(item);
		if (i)
		    victim->addItem(i);
	    }
	}

	// Swallow the victim
	// Seems silly to do this on a miss.
	if (defn().swallows && damage)
	{
	    victim->setSwallowed(true);
	    victim->move(pos(), true);
	}

	// Steal something
	if (defn().isthief)
	{
	    ITEM		*item;
	    
	    // Get a random item from the victim
	    item = victim->getRandomItem();
	    if (item)
	    {
		msg_format("%S <steal> %O.", this, item);
		// Theft successful.
		myFleeCount += 30;
		victim->removeItem(item, true);
		addItem(item);
	    }
	}
    }
	
    return true;
}

const char *
MOB::getMeleeVerb() const
{
    ITEM	*w;

    w = lookupItem(ITEM_WEAPON);
    if (!w)
	return defn().melee_verb;

    return glb_weapon_typedefs[w->getWeaponType()].verb;
}

int
MOB::getMeleeDamage() const
{
    DPDF	damage;

    damage = getMeleeDPDF();

    return damage.evaluate();
}

DPDF
MOB::getMeleeDPDF() const
{
    DPDF	dpdf(0);

    ITEM	*weapon;

    weapon = lookupItem(ITEM_WEAPON);
    if (weapon)
    {
	return weapon->getMeleeDPDF();
    }

    dpdf = defn().melee_damage.buildDPDF();

    dpdf *= (defn().melee_chance) / 100.0;

    return dpdf;
}

int
MOB::getRangedDamage() const
{
    DPDF	damage;

    damage = getRangedDPDF();

    return damage.evaluate();
}

DPDF
MOB::getRangedDPDF() const
{
    ITEM	*weapon;

    weapon = lookupItem(ITEM_SPELLBOOK);
    if (weapon)
    {
	return weapon->getRangeDPDF();
    }

    DPDF	dpdf(0);

    dpdf = defn().range_damage.buildDPDF();

    return dpdf;
}

int
MOB::getRangedRange() const
{
    int		range;

    ITEM	*weapon;

    weapon = lookupItem(ITEM_SPELLBOOK);
    if (weapon)
    {
	return weapon->getRangeRange();
    }

    range = defn().range_range;

    return range;
}

void
MOB::getRangedLook(u8 &symbol, ATTR_NAMES &attr) const
{
    symbol = defn().range_symbol;
    attr = (ATTR_NAMES) defn().range_attr;

    // Check out item, if any.
    ITEM		*i;

    i = lookupItem(ITEM_SPELLBOOK);

    if (i)
    {
	symbol = glb_book_typedefs[i->getBookType()].symbol;
	attr = (ATTR_NAMES) glb_book_elementdefs[i->getBookElement()].attr;
    }
}

bool
MOB::actionFire(int dx, int dy)
{
    MOB		*victim;
    int		rangeleft;
    
    // Check for no ranged weapon.
    if (!defn().range_valid && !hasItem(ITEM_SPELLBOOK))
    {
	msg_format("%S <know> no spells!", this);
	return false;
    }

    // No suicide.
    if (!dx && !dy)
    {
	msg_format("%S <decide> not to aim at %O.", this, this);
	return false;
    }
    
    // If swallowed, rather useless.
    if (isSwallowed())
    {
	msg_format("%S do not have enough room inside here.", this);
	return false;
    }

    // Check for sufficient mana.
    if (isAvatar())
    {
	ITEM		*r;

	r = lookupItem(ITEM_SPELLBOOK);
	if (r)
	{
	    if (r->getRangeMana() > getMP())
	    {
		msg_format("%S <lack> sufficent mana.", this);
		return false;
	    }
	}
    }

    // Check for friendly kill.
    victim = pos().traceBullet(getRangedRange(), dx, dy, &rangeleft);

    if (victim && victim->isFriends(this))
    {
	// Not a clear shot!
	if (isAvatar())
	{
	    msg_report(text_lookup("fire", getRawName()));
	    // We have special messages.
	    return false;
	}

	// Avoid friendly fire
	return false;
    }

    // Shooting is interesting.
    clearBoredom();
 
    u8		symbol;
    ATTR_NAMES	attr;
    int		area = 1;
    
    getRangedLook(symbol, attr);

    // Subtract the mana used.
    if (isAvatar())
    {
	ITEM		*r;

	r = lookupItem(ITEM_SPELLBOOK);
	if (r)
	{
	    myMP -= r->getRangeMana();
	    area = r->getRangeArea();
	}
    }
    pos().displayBullet(getRangedRange(),
			dx, dy,
			symbol, attr,
			!isAvatar() || area != 1);


    if (!victim)
    {
	// Shoot at air!
	msg_format("%S <fire> at empty air!", this);
	// But, really, the fireball should explode!
	POS		vpos;
	vpos = pos().traceBulletPos(getRangedRange(), dx, dy, true);

	// Apply damage to everyone in range.
	vpos.fireball(this, area, getRangedDPDF(), symbol, attr); 

	return true;
    }

    // Attemp to kill victim
    msg_format("%S %v %O.", this, defn().range_verb,
			    victim);

    // NOTE: Bad idea to call a function on victim->pos() as
    // that likely will be deleted :>
    POS		vpos = victim->pos();

    // Apply damage to everyone in range.
    vpos.fireball(this, area, getRangedDPDF(), symbol, attr); 
    

    while (isAvatar() && rangeleft && area == 1)
    {
	// Ensure our dx/dy is copacetic.
	vpos.setAngle(pos().angle());
	victim = vpos.traceBullet(rangeleft, dx, dy, &rangeleft);

	if (victim)
	{
	    msg_format("%S %v %O.", this, defn().range_verb,
				    victim);
	    vpos = victim->pos();
	    vpos.fireball(this, area, getRangedDPDF(), symbol, attr); 
	}
	else
	    rangeleft = 0;
    }
    return true;
}

bool
MOB::actionPortalFire(int dx, int dy, int portal)
{
    // No suicide.
    if (!dx && !dy)
    {
	msg_format("%S <decide> not to aim at %O.", this, this);
	return false;
    }
    
    // If swallowed, rather useless.
    if (isSwallowed())
    {
	msg_format("%S do not have enough room inside here.", this);
	return false;
    }

    clearBoredom();

    // Always display now.
 
    u8		symbol = '*';
    ATTR_NAMES	attr;
    int		portalrange = 60;

    attr = ATTR_BLUE;
    if (portal)
	attr = ATTR_ORANGE;
    
    pos().displayBullet(portalrange,
			dx, dy,
			symbol, attr,
			false);

    POS		vpos;
    vpos = pos().traceBulletPos(portalrange, dx, dy, false, false);
    
    if (vpos.tile() != TILE_WALL && vpos.tile() != TILE_PROTOPORTAL)
    {
	msg_format("The portal dissipates at %O.", 0, vpos.defn().legend);
	return true;
    }

    // We want sloppy targeting for portals.
    // This means that given a portal location of # fired at from the south,
    // we want:
    //
    // 1#1
    // 2*2
    //
    // as potential portals.
    //
    // One fired from the south-west
    //  2
    // 1#2
    // *1

    POS		alt[5];

    if (dx && dy)
    {
	alt[0] = vpos;
	alt[1] = vpos.delta(-dx, 0);
	alt[2] = vpos.delta(0, -dy);
	alt[3] = vpos.delta(0, dy);
	alt[4] = vpos.delta(dx, 0);
    }
    else if (dx)
    {
	alt[0] = vpos;
	alt[1] = vpos.delta(0, 1);
	alt[2] = vpos.delta(0, -1);
	alt[3] = vpos.delta(-dx, 1);
	alt[4] = vpos.delta(-dx, -1);
    }
    else
    {
	alt[0] = vpos;
	alt[1] = vpos.delta(1, 0);
	alt[2] = vpos.delta(-1, 0);
	alt[3] = vpos.delta(1, -dy);
	alt[4] = vpos.delta(-1, -dy);
    }

    for (int i = 0; i < 5; i++)
    {
	if (buildPortalAtLocation(alt[i], portal))
	    return true;
    }
    msg_report("The wall proved too unstable to hold a portal.");

    return true;
}

bool
MOB::buildPortalAtLocation(POS vpos, int portal) const
{
    // We want all our portals in the same space.
    vpos.setAngle(0);

    // Check if it is a valid portal pos?
    if (!vpos.prepSquareForDestruction())
    {
	return false;
    }

    // Verify the portal is well formed, ie, three neighbours are now
    // walls and the other is a floor.
    int		dir, floordir = -1;

    for (dir = 0; dir < 4; dir++)
    {
	TILE_NAMES		tile;
	tile = vpos.delta4Direction(dir).tile();
	if (tile == TILE_FLOOR)
	{
	    if (floordir < 0)
		floordir = dir;
	    else
	    {
		// Uh oh.
		return false;

	    }
	}
	else if (tile == TILE_WALL || tile == TILE_PROTOPORTAL)
	{
	    // All good.
	}
	else
	{
	    // Uh oh.
	    return false;
	}
    }

    // Failed to find a proper portal.
    if (floordir < 0)
	return false;

    cerr << "Build user portal dir " << floordir << endl;

    vpos.map()->buildUserPortal(vpos, portal, (floordir+2) & 3);
    return true;
}


bool
MOB::actionWalk(int dx, int dy)
{
    MOB		*victim;
    POS		 t;

    // If we are swallowed, we can't move.
    if (isSwallowed())
	return false;
    
    t = pos().delta(dx, dy);
    
    victim = t.mob();
    if (victim)
    {
	// If we are the avatar, screw being nice!
	if (isAvatar())
	    return actionMelee(dx, dy);
	// If it is thet avatar we are bumping into, screw being
	// nice.
	if (victim->isAvatar())
	    return actionMelee(dx, dy);

	msg_format("%S <bump> into %O.", this, victim);
	// Bump into the mob.
	return false;
    }

    // Check to see if we can move that way.
    if (canMove(t))
    {
	TILE_NAMES	tile;
	
	// Determine if it is a special move..
	tile = t.tile();
	if (t.defn().isdiggable &&
	    !t.defn().ispassable &&	
	    defn().candig)
	{
	    return t.digSquare();
	}

	POS		opos = pos();
	
	// Move!
	move(t);

	// If we are a breeder, chance to reproduce!
	if (defn().breeder)
	{
	    static  int		lastbreedtime = -10;

	    // Only breed every 7 phases at most so one can
	    // always kill off the breeders.
	    if ((lastbreedtime < spd_gettime() - 7) &&
		!rand_choice(50 + pos().map()->getNumMobs()))
	    {
		MOB		*child;
		
		msg_format("%S <reproduce>!", this);
		child = MOB::create(getDefinition());
		child->move(opos);
		lastbreedtime = spd_gettime();
	    }
	}
	
	// If we are the avatar and something is here, pick it up.
	if (isAvatar() && t.item())
	    actionPickup();
	
	return true;
    }
    else
    {
	if (dx && dy && isAvatar())
	{
	    // Try to wall slide, cause we are pretty real time here
	    // and it is frustrating to navigate curvy passages otherwise.
	    if (!rand_choice(2) && canMoveDir(dx, 0, false))
		if (actionWalk(dx, 0))
		    return true;
	    if (canMoveDir(0, dy, false))
		if (actionWalk(0, dy))
		    return true;
	    if (canMoveDir(dx, 0, false))
		if (actionWalk(dx, 0))
		    return true;
	}
	else if ((dx || dy) && isAvatar())
	{
	    // If we have
	    // ..
	    // @#
	    // ..
	    // Moving right we want to slide to a diagonal.
	    int		sdx, sdy;

	    // This bit of code is too clever for its own good!
	    sdx = !dx;
	    sdy = !dy;

	    if (!rand_choice(2) && canMoveDir(dx+sdx, dy+sdy, false))
		if (actionWalk(dx+sdx, dy+sdy))
		    return true;
	    if (canMoveDir(dx-sdx, dy-sdy, false))
		if (actionWalk(dx-sdx, dy-sdy))
		    return true;
	    if (canMoveDir(dx+sdx, dy+sdy, false))
		if (actionWalk(dx+sdx, dy+sdy))
		    return true;
	    
	}

	msg_format("%S <be> blocked by %O.", this, t.defn().legend);
	// Bump into a wall.
	return false;
    }
}

bool
MOB::actionPickup()
{
    ITEM		*item;

    assert(isAvatar());

    item = pos().item();

    if (!item)
    {
	msg_format("%S <grope> the ground foolishly.", this);
	return false;
    }

    // Pick up the item!
    msg_format("%S <pick> up %O.", this, item);

    item->move(POS());

    // Get our current weapon/armour/book
    ITEM		*olditem;

    olditem = lookupItem(item->getDefinition());

    // Some items are instantly used...
    switch (item->getDefinition())
    {
	case ITEM_HEALPOTION:
	{
	    addItem(item);

	    gainHP(15);
	    removeItem(item);
	    delete item;
	    break;
	}

	case ITEM_MANAPOTION:
	{
	    addItem(item);
    
	    gainMP(15);
	    removeItem(item);
	    delete item;
	    break;
	}

	case ITEM_WEAPON:
	case ITEM_SPELLBOOK:
	{
	    if (aiLikeMore(item, olditem) == item)
	    {
		// We want the new item...
		if (olditem)
		{
		    removeItem(olditem);
		    delete olditem;
		}
		addItem(item);
	    }
	    else
	    {
		// Keep the old item.
		msg_format("%S <discard> it as junk.", this);
		
		delete item;
	    }
	    break;
	}
	default:
	    addItem(item);
	    break;
    }

    return true;
}

void
MOB::addItem(ITEM *item)
{
    int			 i;
    
    // Alert the world to our acquirement.
    if (isAvatar())
	if (item->defn().gaintxt)
	    msg_format(item->defn().gaintxt,
			this, item);

    // First, check to see if we can merge...
    for (i = 0; i < myInventory.entries(); i++)
    {
	if (item->canStackWith(myInventory(i)))
	{
	    myInventory(i)->combineItem(item);
	    delete item;
	    return;
	}
    }

    // Brand new item.
    myInventory.append(item);
}

void
MOB::removeItem(ITEM *item, bool quiet)
{
    // Alert the world to our acquirement.
    if (isAvatar() && !quiet)
	if (item->defn().losetxt)
	    msg_format(item->defn().losetxt,
			this, item);

    myInventory.removePtr(item);
}

void
MOB::loseTempItems()
{
    int		i;
    ITEM	*item;

    for (i = myInventory.entries(); i --> 0;)
    {
	item = myInventory(i);
	if (item->getTimer() >= 0)
	{
	    // All timed items are temporary.
	    removeItem(item, true);
	    delete item;
	}
    }
}

void
MOB::save(ostream &os) const
{
    int		val;

    val = myDefinition;
    os.write((const char *) &val, sizeof(int));

    myPos.save(os);
    myTarget.save(os);
    myHome.save(os);

    os.write((const char *) &myHP, sizeof(int));
    os.write((const char *) &myMP, sizeof(int));
    os.write((const char *) &myAIState, sizeof(int));
    os.write((const char *) &myFleeCount, sizeof(int));
    os.write((const char *) &myBoredom, sizeof(int));
    os.write((const char *) &myNumDeaths, sizeof(int));

    val = isSwallowed();
    os.write((const char *) &val, sizeof(int));

    int			 numitem;
    int			 i;

    numitem = myInventory.entries();
    os.write((const char *) &numitem, sizeof(int));

    for (i = 0; i < myInventory.entries(); i++)
    {
	myInventory(i)->save(os);
    }
}

MOB *
MOB::load(istream &is)
{
    int		 val, num, i;
    MOB		*mob;

    mob = new MOB();

    is.read((char *)&val, sizeof(int));
    mob->myDefinition = (MOB_NAMES) val;

    mob->myPos.load(is);
    mob->myTarget.load(is);
    mob->myHome.load(is);

    is.read((char *)&mob->myHP, sizeof(int));
    is.read((char *)&mob->myMP, sizeof(int));
    is.read((char *)&mob->myAIState, sizeof(int));
    is.read((char *)&mob->myFleeCount, sizeof(int));
    is.read((char *)&mob->myBoredom, sizeof(int));
    is.read((char *)&mob->myNumDeaths, sizeof(int));

    is.read((char *)&val, sizeof(int));
    mob->setSwallowed(val ? true : false);

    is.read((char *)&num, sizeof(int));

    for (i = 0; i < num; i++)
    {
	mob->myInventory.append(ITEM::load(is));
    }

    return mob;
}

bool
MOB::hasVisibleEnemies() const
{
    PTRLIST<MOB *> list;

    getVisibleEnemies(list);
    if (list.entries())
	return true;
    return false;
}

void
MOB::getVisibleEnemies(PTRLIST<MOB *> &list) const
{
    // If we are blind, we can't see anyone.
    if (hasItem(ITEM_BLIND))
	return;

    // We only care about the non-avatar case now.
    if (isAvatar())
	return;

    // Check if we are in FOV.  If so, the avatar is visible.
    if (getAvatar())
    {
	// Ignore none hostile
	if (isFriends(getAvatar()))
	    return;

	if (pos().isFOV())
	{
	    list.append(getAvatar());
	}
    }
}
