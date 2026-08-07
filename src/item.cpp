/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        item.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "item.h"

#include <assert.h>

#include <iostream>
using namespace std;

//
// Item Definitions
//

ITEM::ITEM()
{
    myDefinition = (ITEM_NAMES) 0;
    myCount = 1;
    myTimer = -1;

    // These are irrelevant for most items.
    myType = myMaterial = myModifier = 0;
}

ITEM::~ITEM()
{
    pos().removeItem(this);
}

void
ITEM::move(POS pos)
{
    myPos.removeItem(this);
    myPos = pos;
    myPos.addItem(this);
}

ITEM *
ITEM::copy() const
{
    ITEM *item;

    item = new ITEM();

    *item = *this;

    return item;
}

ITEM *
ITEM::create(ITEM_NAMES item, int depth)
{
    ITEM	*i;

    assert(item >= ITEM_NONE && item < NUM_ITEMS);

    i = new ITEM();

    i->myDefinition = item;
    i->myTimer = glb_itemdefs[item].timer;

    // Special case for weapon/armour/spellbooks.
    switch (i->myDefinition)
    {
	case ITEM_WEAPON:
	    i->buildWeapon(depth);
	    break;

	case ITEM_SPELLBOOK:
	    i->buildSpellBook(depth);
	    break;
    }

    return i;
}

void
ITEM::buildWeapon(int depth)
{
    int		numchoice = 0;
    int		totalpoints;
    WEAPON_TYPE_NAMES		wtype;
    WEAPON_MATERIAL_NAMES	wmat;
    WEAPON_MODIFIER_NAMES	wmod;

    // we start with a default.  We then search for anything within
    // our depth allowance, picking with replacement each time.  We
    // pick uniformally from all eligible items.  An item is eligible
    // if
    // totalpoints - depth*2 < rand(5);

    myType = WEAPON_TYPE_KNIFE;
    numchoice = 0;
    FOREACH_WEAPON_TYPE(wtype)
    {
	totalpoints = glb_weapon_typedefs[wtype].power;
	totalpoints += glb_weapon_typedefs[wtype].accuracy;
	totalpoints += glb_weapon_typedefs[wtype].consistency;
	totalpoints -= depth * 2;
	if (totalpoints < rand_choice(5))
	{
	    numchoice++;
	    if (!rand_choice(numchoice))
		myType = wtype;
	}
    }

    myMaterial = WEAPON_MATERIAL_WOOD;
    numchoice = 0;
    FOREACH_WEAPON_MATERIAL(wmat)
    {
	totalpoints =  glb_weapon_materialdefs[wmat].power;
	totalpoints += glb_weapon_materialdefs[wmat].accuracy;
	totalpoints += glb_weapon_materialdefs[wmat].consistency;
	totalpoints -= depth * 2;
	if (totalpoints < rand_choice(5))
	{
	    numchoice++;
	    if (!rand_choice(numchoice))
		myMaterial = wmat;
	}
    }

    myModifier = WEAPON_MODIFIER_BROKEN;
    numchoice = 0;
    FOREACH_WEAPON_MODIFIER(wmod)
    {
	totalpoints =  glb_weapon_modifierdefs[wmod].power;
	totalpoints += glb_weapon_modifierdefs[wmod].accuracy;
	totalpoints += glb_weapon_modifierdefs[wmod].consistency;
	totalpoints -= depth * 2;
	if (totalpoints < rand_choice(5))
	{
	    numchoice++;
	    if (!rand_choice(numchoice))
		myModifier = wmod;
	}
    }
}

void
ITEM::buildSpellBook(int depth)
{
    int		numchoice = 0;
    int		totalpoints;
    BOOK_TYPE_NAMES	btype;
    BOOK_MATERIAL_NAMES	bmat;
    BOOK_ELEMENT_NAMES	belem;

    // we start with a default.  We then search for anything within
    // our depth allowance, picking with replacement each time.  We
    // pick uniformally from all eligible items.  An item is eligible
    // if
    // totalpoints - depth*2 < rand(5);

    myType = BOOK_TYPE_BOLT;
    numchoice = 0;
    FOREACH_BOOK_TYPE(btype)
    {
	totalpoints =  glb_book_typedefs[btype].power;
	totalpoints += glb_book_typedefs[btype].consistency;
	totalpoints += glb_book_typedefs[btype].range;
	totalpoints += glb_book_typedefs[btype].area * 2;
	totalpoints -= depth * 2;
	if (totalpoints < rand_choice(5))
	{
	    numchoice++;
	    if (!rand_choice(numchoice))
		myType = btype;
	}
    }

    myMaterial = BOOK_MATERIAL_PAPYRUS;
    numchoice = 0;
    FOREACH_BOOK_MATERIAL(bmat)
    {
	totalpoints =  glb_book_materialdefs[bmat].power;
	totalpoints += glb_book_materialdefs[bmat].consistency;
	totalpoints += glb_book_materialdefs[bmat].range;
	totalpoints += glb_book_materialdefs[bmat].area * 2;
	totalpoints -= depth * 2;
	if (totalpoints < rand_choice(5))
	{
	    numchoice++;
	    if (!rand_choice(numchoice))
		myMaterial = bmat;
	}
    }

    myModifier = BOOK_ELEMENT_CHILL;
    numchoice = 0;
    FOREACH_BOOK_ELEMENT(belem)
    {
	totalpoints =  glb_book_elementdefs[belem].power;
	totalpoints += glb_book_elementdefs[belem].consistency;
	totalpoints += glb_book_elementdefs[belem].range;
	totalpoints += glb_book_elementdefs[belem].area * 2;
	totalpoints -= depth * 2;
	if (totalpoints < rand_choice(5))
	{
	    numchoice++;
	    if (!rand_choice(numchoice))
		myModifier = belem;
	}
    }
}

ITEM *
ITEM::createRandom(int depth)
{
    ITEM_NAMES		item;

    item = itemFromHash(rand_choice(65536*32767));
    return create(item, depth);
}

ITEM_NAMES
ITEM::itemFromHash(unsigned int hash)
{
    int		totalrarity, rarity;
    int		i;

    // Find total rarity of items
    totalrarity = 0;
    for (i = ITEM_NONE; i < NUM_ITEMS; i++)
    {
	rarity = defn((ITEM_NAMES)i).rarity;
	totalrarity += rarity;
    }

    hash %= totalrarity;
    for (i = ITEM_NONE; i < NUM_ITEMS; i++)
    {
	rarity = defn((ITEM_NAMES)i).rarity;

	if (hash > (unsigned) rarity)
	    hash -= rarity;
	else
	    break;
    }
    return (ITEM_NAMES) i;
}

VERB_PERSON
ITEM::getPerson() const
{
    return VERB_IT;
}

BUF
ITEM::getName() const
{
    BUF		buf;

    // Handle special cases.
    switch (getDefinition())
    {
	case ITEM_WEAPON:
	{
	    buf.sprintf("%s %s %s",
		    glb_weapon_modifierdefs[myModifier].name,
		    glb_weapon_materialdefs[myMaterial].name,
		    glb_weapon_typedefs[myType].name);
	    break;
	}
	case ITEM_SPELLBOOK:
	{
	    buf.sprintf("%s spellbook of %s %s",
		    glb_book_materialdefs[myMaterial].name,
		    glb_book_elementdefs[myModifier].name,
		    glb_book_typedefs[myType].name);
	    break;
	}
    }

    if (!buf.isstring())
    {
	if (getTimer() >= 0)
	{
	    // A timed item...
	    buf.sprintf("%s (%d)", defn().name, getTimer());
	}
	else
	{
	    // A normal item.
	    buf.reference(defn().name);
	}
    }
    return gram_createcount(buf, myCount, false);
}

BUF
ITEM::getDetailedDescription() const
{
    BUF			buf;

    switch (getDefinition())
    {
	case ITEM_WEAPON:
	{
	    int		p, a, c;
	    int		min, q1, q2, q3, max;
	    DPDF	damage;

	    damage = getMeleeDPDF();
	    // Only count hits.
	    damage.applyGivenGreaterThan(0);
	    damage.getQuartile(min, q1, q2, q3, max);

	    getWeaponStats(p, a, c);
	    buf.sprintf("Accuracy: %d\nDamage: \n%d..%d..%d..%d..%d\n",
			    a, min, q1, q2, q3, max);
	    return buf;
	}
	case ITEM_SPELLBOOK:
	{
	    int		p, c, r, a, m;
	    int		min, q1, q2, q3, max;
	    DPDF	damage;

	    damage = getRangeDPDF();
	    // Spells always hit
	    damage.getQuartile(min, q1, q2, q3, max);

	    getRangeStats(r, p, c, a, m);
	    buf.sprintf(
		        "Area: %d Range: %d\n"
		        "Mana: %d\n"	
		        "Damage:\n%d..%d..%d..%d..%d\n",
			    a, r, m,
			    min, q1, q2, q3, max);
	    return buf;
	}
	default:
	    return 0;
    }
}

void
ITEM::getLook(u8 &symbol, ATTR_NAMES &attr) const
{
    symbol = defn().symbol;
    attr = (ATTR_NAMES) defn().attr;
}

bool
ITEM::canStackWith(const ITEM *stack) const
{
    if (getDefinition() != stack->getDefinition())
	return false;

    // Disable stacking of weapons/books/armour as we want
    // to be able to delete them easily
    if (defn().unstackable)
	return false;

    // Check type, material, modifier.
    if (myType != stack->myType ||
	myMaterial != stack->myMaterial ||
	myModifier != stack->myModifier)
    {
	return false;
    }

    // No reason why not...
    return true;
}

void
ITEM::combineItem(const ITEM *item)
{
    // Untimed items stack.
    if (getTimer() < 0)
	myCount += item->getStackCount();
    // Timed items add up charges.
    if (getTimer() >= 0)
    {
	assert(item->getTimer() >= 0);
	myTimer += item->getTimer();
    }

    assert(myCount >= 0);
    if (myCount < 0)
	myCount = 0;
}

bool
ITEM::runHeartbeat()
{
    if (myTimer >= 0)
    {
	if (!myTimer)
	    return true;
	myTimer--;
    }
    return false;
}

void
ITEM::save(ostream &os) const
{
    int		val;

    val = myDefinition;
    os.write((const char *) &val, sizeof(int));

    myPos.save(os);

    os.write((const char *) &myCount, sizeof(int));
    os.write((const char *) &myTimer, sizeof(int));

    os.write((const char *) &myType, sizeof(u8));
    os.write((const char *) &myMaterial, sizeof(u8));
    os.write((const char *) &myModifier, sizeof(u8));
}

ITEM *
ITEM::load(istream &is)
{
    int		val;
    ITEM	*i;

    i = new ITEM();

    is.read((char *)&val, sizeof(int));
    i->myDefinition = (ITEM_NAMES) val;

    i->myPos.load(is);

    is.read((char *)&i->myCount, sizeof(int));
    is.read((char *)&i->myTimer, sizeof(int));

    is.read((char *) &i->myType, sizeof(u8));
    is.read((char *) &i->myMaterial, sizeof(u8));
    is.read((char *) &i->myModifier, sizeof(u8));

    return i;
}

void
ITEM::getWeaponStats(int &power, int &accuracy, int &consistency) const
{
    power = accuracy = consistency = 0;

    if (getDefinition() != ITEM_WEAPON)
	return;

    power = glb_weapon_typedefs[myType].power;
    power += glb_weapon_materialdefs[myMaterial].power;
    power += glb_weapon_modifierdefs[myModifier].power;
    accuracy = glb_weapon_typedefs[myType].accuracy;
    accuracy += glb_weapon_materialdefs[myMaterial].accuracy;
    accuracy += glb_weapon_modifierdefs[myModifier].accuracy;
    consistency = glb_weapon_typedefs[myType].consistency;
    consistency += glb_weapon_materialdefs[myMaterial].consistency;
    consistency += glb_weapon_modifierdefs[myModifier].consistency;
}

DPDF
ITEM::getMeleeDPDF() const
{
    if (getDefinition() != ITEM_WEAPON)
	return DPDF(0);

    // Compute our power, accuracy, and consistency.
    int		power, accuracy, consistency, i;

    getWeaponStats(power, accuracy, consistency);

    // Power produces a pure DPDF
    DPDF		damage(0);

    damage += DPDF(0, power*2);

    // Add in the consistency bonus.  This is also comparable to power,
    // but is the max of three uniform variates.
    DPDF		con(0);
    for (i = 0; i < 3; i++)
    {
	con.max(con, DPDF(0, consistency));
    }

    // Add in consistency bonus.
    damage += con;

    // Now, scale by the accuracy.
    // Max accuracy is eldritch diamond spear
    //			10	  15	15
    // Which is 40.  We thus give an accuracy of
    // 20% + accuracy * 2.
    // This is way too mean.
    // Use base of 50, same calculation, but allow accuracy > 100%
    // to allow double hit.
    double	tohit;

    tohit = (accuracy*2 + 45) / 100.0;
    if (tohit > 1.0)
    {
	DPDF		critical;

	critical = damage;
	critical *= (tohit - 1.0);
	damage += critical;
    }
    else
	damage *= tohit;

    return damage;
}

void
ITEM::getRangeStats(int &range, int &power, int &consistency, int &area, int &mana) const
{
    range = power = consistency = mana = 0;

    if (getDefinition() != ITEM_SPELLBOOK)
	return;

    range = glb_book_typedefs[myType].range;
    range += glb_book_materialdefs[myMaterial].range;
    range += glb_book_elementdefs[myModifier].range;
    power = glb_book_typedefs[myType].power;
    power += glb_book_materialdefs[myMaterial].power;
    power += glb_book_elementdefs[myModifier].power;
    area = glb_book_typedefs[myType].area;
    area += glb_book_materialdefs[myMaterial].area;
    area += glb_book_elementdefs[myModifier].area;
    consistency = glb_book_typedefs[myType].consistency;
    consistency += glb_book_materialdefs[myMaterial].consistency;
    consistency += glb_book_elementdefs[myModifier].consistency;
    mana = glb_book_typedefs[myType].mana;
    mana += glb_book_materialdefs[myMaterial].mana;
    mana += glb_book_elementdefs[myModifier].mana;
}

int
ITEM::getRangeRange() const
{
    // Compute our power, accuracy, and consistency.
    int		range, d;

    getRangeStats(range, d, d, d, d);

    return range;
}

int
ITEM::getRangeMana() const
{
    if (getDefinition() != ITEM_SPELLBOOK)
	return 0;

    // Compute our power, accuracy, and consistency.
    int		mana, d;

    getRangeStats(d, d, d, d, mana);

    return mana;
}

int
ITEM::getRangeArea() const
{
    // Compute our power, accuracy, and consistency.
    int		area, d;

    getRangeStats(d, d, d, area, d);

    return area;
}
DPDF
ITEM::getRangeDPDF() const
{
    if (getDefinition() != ITEM_SPELLBOOK)
	return DPDF(0);

    // Compute our power, accuracy, and consistency.
    int		power, consistency, i;

    getRangeStats(i, power, consistency, i, i);

    // Power produces a pure DPDF
    DPDF		damage(0);

    damage += DPDF(0, power*2);

    // Add in the consistency bonus.  This is also comparable to power,
    // but is the max of three uniform variates.
    DPDF		con(0);
    for (i = 0; i < 3; i++)
    {
	con.max(con, DPDF(0, consistency));
    }

    // Add in consistency bonus.
    damage += con;

    return damage;
}
