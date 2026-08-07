/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        item.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __item__
#define __item__

#include "glbdef.h"

#include "dpdf.h"
#include "grammar.h"
#include "map.h"
#include "buf.h"

#include <iostream>
using namespace std;

class ITEM
{
public:
		~ITEM();

    static ITEM *create(ITEM_NAMES item, int depth = 0);

    static ITEM *createRandom(int depth);
    static ITEM_NAMES itemFromHash(unsigned hash);

    // Constructs the relative specific type from the generic.
    void	 buildWeapon(int depth);
    void	 buildSpellBook(int depth);

    ITEM	*copy() const;

    ITEM_NAMES	 getDefinition() const { return myDefinition; }
		
    VERB_PERSON	 getPerson() const;
    BUF		 getName() const;

    // Returns 0 if no detailed description.  Returns a multi-line
    // buffer prefixed with +-
    BUF		 getDetailedDescription() const;

    const ITEM_DEF	&defn() const { return defn(getDefinition()); }
    static const ITEM_DEF &defn(ITEM_NAMES item) { return glb_itemdefs[item]; }

    void	 getLook(u8 &symbol, ATTR_NAMES &attr) const;

    const POS   &pos() const { return myPos; }
    void	 move(POS pos);
    void	 setMap(MAP *map) { myPos.setMap(map); }

    bool	 canStackWith(const ITEM *stack) const;
    void	 combineItem(const ITEM *item);
    int		 getStackCount() const { return myCount; }
    void	 decStackCount() { myCount--; }

    // -1 for things without a count down.
    int		 getTimer() const { return myTimer; }

    // Returns true if should self destruct.
    bool	 runHeartbeat();

    DPDF	 getMeleeDPDF() const;

    void	 getWeaponStats(int &power, int &accuracy, int &consistency) const;

    DPDF	 getRangeDPDF() const;
    int		 getRangeRange() const;
    int		 getRangeMana() const;
    int		 getRangeArea() const;

    void	 getRangeStats(int &range, int &power, int &consistency,
				int &area, int &mana) const;

    void	 save(ostream &os) const;
    static ITEM	*load(istream &is);

    BOOK_TYPE_NAMES	getBookType() const { return (BOOK_TYPE_NAMES) myType; }
    BOOK_ELEMENT_NAMES	getBookElement() const { return (BOOK_ELEMENT_NAMES) myModifier; }

    WEAPON_TYPE_NAMES	getWeaponType() const { return (WEAPON_TYPE_NAMES) myType; }
    
protected:
		 ITEM();

    ITEM_NAMES	 myDefinition;

    POS		 myPos;
    int		 myCount;
    int		 myTimer;

    // THese are only valid for weapons/armour/spellbooks.
    u8		 myType, myMaterial, myModifier;
};

#endif

