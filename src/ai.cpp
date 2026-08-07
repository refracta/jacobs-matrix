/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        ai.cpp ( Save Scummer Library, C++ )
 *
 * COMMENTS:
 */

#include "mob.h"

#include "map.h"
#include "msg.h"
#include "text.h"
#include "speed.h"
#include "item.h"

bool
MOB::aiForcedAction()
{
    // Check for the phase...
    PHASE_NAMES		phase;

    phase = spd_getphase();

    if (!alive())
	return false;

    // On normal & slow phases, items get heartbeat.
    if (phase == PHASE_NORMAL || phase == PHASE_SLOW)
    {
	// We don't want magic rings in other people's pockets
	// to expire.
	if (getDefinition() == MOB_AVATAR)
	{
	    int i;
	    for (i = myInventory.entries(); i --> 0;)
	    {
		if (myInventory(i)->runHeartbeat())
		{
		    ITEM		*item = myInventory(i);
		    removeItem(item);
		    delete item;
		}
	    }
	}

	// Regenerate if we are capable of it
	if (defn().isregenerate || hasItem(ITEM_REGENRING))
	{
	    gainHP(1);
	}

	// Apply damage if inside.
	if (isSwallowed())
	{
	    msg_format("%S <be> digested!", this);

	    // Make it a force action if it kills us.
	    if (applyDamage(0, rand_range(1, 4)))
		return true;
	}
    }

    switch (phase)
    {
	case PHASE_FAST:
	    if (hasItem(ITEM_SPEEDBOOTS))
		return false;
	    
	    // Not fast, no phase
	    if (defn().isfast)
		return false;
	    return true;

	case PHASE_QUICK:
	    // Not quick, no phase!
	    if (hasItem(ITEM_QUICKBOOST))
		return false;
	    return true;

	case PHASE_SLOW:
	    if (defn().isslow)
		return true;
	    if (hasItem(ITEM_SLOW))
		return true;
	    break;

	case PHASE_NORMAL:
	    break;
    }
    return false;
}

bool
MOB::aiDoAI()
{
    // Rebuild our home location.
    if (!myHome.valid())
    {
	myHome = pos();
    }

    if (myFleeCount > 0)
    {
	myFleeCount--;
	if (!isAvatar() && aiFleeFromAvatar())
	    return true;
    }

    myBoredom++;
    if (myBoredom > 1000)
	myBoredom = 1000;

    switch (getAI())
    {
	case AI_NONE:
	    // Just stay put!
	    return true;

	case AI_STAYHOME:
	    // If we are at home stay put.
	    if (pos() == myHome)
		return true;

	    // Otherwise, go home.
	    // FALL THROUGH

	case AI_HOME:
	    // Move towards our home square.
	    int		dist;

	    dist = pos().dist(myHome);

	    // If we can shoot the avatar or charge him, do so.
	    if (aiRangeAttack())
		return true;
	    if (aiCharge(getAvatar(), AI_CHARGE))
		return true;

	    // A good chance to just stay put.
	    if (rand_chance(70))
	    {
		return actionBump(0, 0);
	    }

	    if (rand_choice(dist))
	    {
		// Try to home.
		if (aiMoveTo(myHome))
		    return true;
	    }
	    // Default to wandering
	    return aiRandomWalk();

	case AI_ORTHO:
	    if (aiRangeAttack())
		return true;
	    if (aiCharge(getAvatar(), AI_CHARGE, true))
		return true;
	    // Default to wandering
	    return aiRandomWalk(true);

	case AI_CHARGE:
	    if (aiRangeAttack())
		return true;
	    if (aiCharge(getAvatar(), AI_CHARGE))
		return true;
	    // Default to wandering
	    return aiRandomWalk();

	case AI_PATHTO:
	    if (aiKillPathTo(getAvatar()))
		return true;
		    
	    // Default to wandering
	    return aiRandomWalk();

	case AI_FLANK:
	    if (aiRangeAttack())
		return true;
	    if (aiCharge(getAvatar(), AI_FLANK))
		return true;
	    // Default to wandering
	    return aiRandomWalk();

	case AI_RANGECOWARD:
	    // If we can see avatar, shoot at them!
	    // We keep ourselves at range if possible, unless
	    // retreat is blocked, in which case we will melee.
	    if (pos().isFOV())
	    {
		int		 dist, dx, dy;

		if (aiAcquireAvatar())
		{
		    pos().dirTo(myTarget, dx, dy);
		    dist = pos().dist(myTarget);

		    if (dist == 1)
		    {
			// We are in melee range.  Try to flee.
			if (aiFleeFrom(myTarget))
			    return true;

			// Failed to flee.  Kill!
			return actionMelee(dx, dy);
		    }

		    // Try ranged attack.
		    if (aiRangeAttack())
			return true;

		    // Failed range attack.  If outside of range, charge.
		    if (dist > getRangedRange())
		    {
			if (aiCharge(getAvatar(), AI_FLANK))
			    return true;
		    }

		    // Otherwise, wander within the range hoping to line up.
		    // Trying to double think the human is likely pointless
		    // as they'll be trying to avoid lining up to.
		    return aiRandomWalk();
		}
	    }
		
	    if (aiCharge(getAvatar(), AI_FLANK))
		return true;

	    // Default to wandering
	    return aiRandomWalk();

	    
	case AI_COWARD:
	    if (aiRangeAttack())
		return true;

	    if (aiFleeFromAvatar())
		return true;
	    // Failed to flee, cornered, so charge!
	    if (aiCharge(getAvatar(), AI_CHARGE))
		return true;

	    return aiRandomWalk();

	case AI_MOUSE:
	    return aiDoMouse();

	case AI_STRAIGHTLINE:
	    return aiStraightLine();
    }

    return false;
}


double
avatar_weight(ITEM *item)
{
    ITEM		*weapon, *book;

    weapon = item;
    book = item;
    if (weapon->getDefinition() == ITEM_WEAPON)
    {
	int		ip, ia, ic;
	double	weight;
	
	weapon->getWeaponStats(ip, ia, ic);

	// These are conveniently normalized.
	weight = ip * glb_roledefs[MOB::avatarRole()].w_power;
	weight += ia * glb_roledefs[MOB::avatarRole()].w_accuracy;
	weight += ic * glb_roledefs[MOB::avatarRole()].w_consistency;
	
	return weight;
    }
    if (book->getDefinition() == ITEM_SPELLBOOK)
    {
	int		ir, ip, ia, ic, im;
	double	weight;
	
	book->getRangeStats(ir, ip, ic, ia, im);

	// These are not well normalized;
	// We want things in a 0..20 range, so we double
	// the weight of range, 20-5xmana, and 7*area
	weight = ir * glb_roledefs[MOB::avatarRole()].b_range * 2;
	weight += ip * glb_roledefs[MOB::avatarRole()].b_power;
	weight += ic * glb_roledefs[MOB::avatarRole()].b_consistency;
	weight += ia * glb_roledefs[MOB::avatarRole()].b_area * 7;
	weight += 20 - im * glb_roledefs[MOB::avatarRole()].b_mana * 5;
	
	return weight;
    }
    return 0.0;
}

ITEM *
MOB::aiLikeMore(ITEM *a, ITEM *b) const
{
    // Trivial case
    if (!a) return b;
    if (!b) return a;
    
    assert(a->getDefinition() == b->getDefinition());

    if (avatar_weight(b) > avatar_weight(a))
	return b;
    // Default to a.
    return a;
}

bool
MOB::aiDoMouse()
{
    int		lastdir, dir, newdir;
    int		dx, dy;
    bool	found = false;
    int		i;

    // Least three bits track our last direction.
    lastdir = myAIState & 7;

    // We want to try and track a wall.  We want this wall to
    // be on our right hand side.

    // Take a look at right hand square.
    rand_angletodir((lastdir + 2) & 7, dx, dy);
    
    if (!canMoveDir(dx, dy, true))
    {
	// Got a wall to the right.  Try first available direction.
	// Because we want to hug using diagonals, we try the forward
	// and right first.
	dir = (lastdir+1) & 7;
    }
    else
    {
	// No wall on right.  Try and go straight forward by default.
	dir = lastdir;
    }

    // If everyway is blocked, bump straight forward.
    newdir = lastdir;
    for (i = 0; i < 8; i++)
    {
	rand_angletodir(dir, dx, dy);
	if (canMoveDir(dx, dy, true))
	{
	    newdir = dir;
	    break;
	}
	// Keeping wall on right means turning left first!
	dir--;
	dir &= 7;
    }

    if (newdir == -1)
	newdir = lastdir;
    else
	found = true;

    // Store our last direction.
    myAIState &= ~7;
    myAIState |= newdir;

    if (found)
    {
	return actionBump(dx, dy);
    }

    // Mouse is cornered!  Return to normal AI.
    return false;
}

bool
MOB::aiStraightLine()
{
    int		lastdir;
    int		dx, dy;
    MOB		*target;
    POS		t;

    // Least three bits track our last direction.
    lastdir = myAIState & 7;

    // Bump & go.

    // Take a look at right hand square.
    rand_angletodir(lastdir, dx, dy);

    t = pos().delta(dx, dy);
    
    // If target is avatar, run into him!
    target = t.mob();
    if (target && !isFriends(target))
    {
	return actionBump(dx, dy);
    }
    
    if (!canMove(t, true))
    {
	// Can't go there!  Pick new direction.
	// Do not want same direction again.
	lastdir += rand_choice(7) + 1;
	lastdir &= 7;
    }
    myAIState = lastdir;
    rand_angletodir(lastdir, dx, dy);
    t = pos().delta(dx, dy);
    
    // If target is avatar, run into him!
    target = t.mob();
    if (target && !isFriends(target))
    {
	return actionBump(dx, dy);
    }

    // Bump failed.
    if (!canMove(t, true))
	return false;

    // Move forward!
    return actionBump(dx, dy);
}

bool
MOB::aiFleeFromAvatar()
{
    if (aiAcquireAvatar())
    {
	if (aiFleeFrom(myTarget))
	    return true;
    }

    return false;
}

bool
MOB::aiFleeFrom(POS goal)
{
    int		dx, dy;
    int		angle, resultangle = 0, i;
    int		choice = 0;

    pos().dirTo(goal, dx, dy);
    dx = -dx;
    dy = -dy;
    
    angle = rand_dirtoangle(dx, dy);

    // 3 ones in same direction get equal chance.
    for (i = angle-1; i <= angle+1; i++)
    {
	rand_angletodir(i, dx, dy);
	if (canMoveDir(dx, dy))
	{
	    choice++;
	    if (!rand_choice(choice))
		resultangle = i;
	}
    }

    if (!choice)
    {
	// Try a bit more desperately...
	for (i = angle-2; i <= angle+2; i += 4)
	{
	    rand_angletodir(i, dx, dy);
	    if (canMoveDir(dx, dy))
	    {
		choice++;
		if (!rand_choice(choice))
		    resultangle = i;
	    }
	}
    }

    if (choice)
    {
	// Move in the direction
	rand_angletodir(resultangle, dx, dy);
	return actionBump(dx, dy);
    }

    // Failed
    return false;
}

bool
MOB::aiAcquireAvatar()
{
    MOB		*a;
    a = getAvatar();

    return aiAcquireTarget(a);
}

bool
MOB::aiAcquireTarget(MOB *a)
{
    if (a && a->isSwallowed())
    {
	myTarget = POS();
	return false;
    }
    // If we can see avatar, charge at them!  Otherwise, move
    // to our target if set.  Otherwise random walk.
    if (a && 
	a->pos().isFOV() &&
	pos().isFOV())
    {
	int		dist;
	// Determine distance
	dist = pos().dist(a->pos());
	if (dist <= defn().sightrange)
	{
	    myTarget = a->pos();
	}
    }
    else
    {
	// Random chance of forgetting avatar location.
	// Also, if current location is avatar location, we forget it.
	if (pos() == myTarget)
	    myTarget = POS();
	if (!rand_choice(10))
	    myTarget = POS();
    }

    return myTarget.valid();
}

bool
MOB::aiRandomWalk(bool orthoonly)
{
    int		dx, dy;
    int		angle, resultangle = 0;
    int		choice = 0;

    for (angle = 0; angle < 8; angle++)
    {
	rand_angletodir(angle, dx, dy);
	if (canMoveDir(dx, dy))
	{
	    choice++;
	    if (!rand_choice(choice))
		resultangle = angle;
	}
	if (orthoonly)
	    angle++;
    }

    if (choice)
    {
	// Move in the direction
	rand_angletodir(resultangle, dx, dy);
	return actionWalk(dx, dy);
    }

    // Failed
    return false;
}

bool
MOB::aiCharge(MOB *foe, AI_NAMES aitype, bool orthoonly)
{
    if (aiAcquireTarget(foe))
    {
	if (aitype == AI_FLANK)
	{
	    if (aiFlankTo(myTarget))
		return true;
	}
	else
	{
	    if (aiMoveTo(myTarget, orthoonly))
		return true;
	}
    }

    return false;
}

bool
MOB::aiRangeAttack(MOB *target)
{
    // See if we have a ranged attack at all!
    if (!defn().range_valid && !hasItem(ITEM_SPELLBOOK))
	return false;

    // Check for sufficient mana.
    if (isAvatar())
    {
	ITEM		*r;

	r = lookupItem(ITEM_SPELLBOOK);
	if (r)
	{
	    if (r->getRangeMana() > getMP())
	    {
		return false;
	    }
	}
    }

    int		 dx, dy;
    MOB		*a = getAvatar(), *v;

    // Override the target from the avatar...
    if (target)
	a = target;

    if (!a)
	return false;

    pos().dirTo(a->pos(), dx, dy);

    // If we are in melee range, don't use our ranged attack!
    if (pos().dist(a->pos()) <= 1)
    {
	return false;
    }

    // Try ranged attack.
    v = pos().traceBullet(getRangedRange(), dx, dy);
    if (v == a)
    {
	// Potential range attack.
	return actionFire(dx, dy);
    }

    return false;
}

bool
MOB::aiMoveTo(POS t, bool orthoonly)
{
    int		dx, dy, dist;
    int		angle, resultangle = 0, i;
    int		choice = 0;

    pos().dirTo(t, dx, dy);

    if (orthoonly && dx && dy)
    {
	// Ensure source is orthogonal.
	if (rand_choice(2))
	    dx = 0;
	else
	    dy = 0;
    }

    dist = pos().dist(t);

    if (dist == 1)
    {
	// Attack!
	if (canMoveDir(dx, dy, false))
	    return actionBump(dx, dy);
	return false;
    }

    // Move in general direction, preferring a straight line.
    angle = rand_dirtoangle(dx, dy);

    for (i = 0; i <= 2; i++)
    {
	if (orthoonly && (i & 1))
	    continue;

	rand_angletodir(angle-i, dx, dy);
	if (canMoveDir(dx, dy))
	{
	    choice++;
	    if (!rand_choice(choice))
		resultangle = angle-i;
	}
	
	rand_angletodir(angle+i, dx, dy);
	if (canMoveDir(dx, dy))
	{
	    choice++;
	    if (!rand_choice(choice))
		resultangle = angle+i;
	}

	if (choice)
	{
	    rand_angletodir(resultangle, dx, dy);

	    // If wew can leap, maybe leap?
	    if (defn().canleap)
	    {
		if (dist > 2 &&
		    canMoveDir(2*dx, 2*dy))
		{
		    if (pos().isFOV())
		    {
			msg_format("%S <leap>.", this);
		    }
		    dx *= 2;
		    dy *= 2;
		}
	    }
	    
	    return actionWalk(dx, dy);
	}
    }

    return false;
}

bool
MOB::aiKillPathTo(MOB *target)
{
    if (!target)
	return false;
    
    return aiPathFindTo(target->pos());
}

bool
MOB::aiPathFindTo(POS goal)
{
    int		dx, dy, ndx, ndy;
    int		curdist, dist;

    bool	found;

    // see if already there.
    if (pos() == goal)
	return false;

    // Really, really, inefficient.
    pos().map()->buildDistMap(goal);

    curdist = pos().getDistance();
    
    // We don't care if this square is unreachable, as if it is the
    // square with the mob, almost by definition it is unreachable
    
    found = false;
    FORALL_8DIR(dx, dy)
    {
	dist = pos().delta(dx, dy).getDistance();
	if (dist >= 0 && dist < curdist)
	{
	    found = true;
	    ndx = dx;
	    ndy = dy;
	    curdist = dist;
	}
    }

    // If we didn't find a direction, abort
    if (!found)
    {
	msg_format("%S cannot find a path there.", this);
	return false;
    }

    // Otherwise, act.
    return actionWalk(ndx, ndy);
}

bool
MOB::aiFlankTo(POS goal)
{
    int		dx, dy, dist;
    int		angle, resultangle = 0, i, j;
    int		choice = 0;

    pos().dirTo(goal, dx, dy);

    dist = pos().dist(goal);

    if (dist == 1)
    {
	// Attack!
	if (canMove(goal, false))
	    return actionBump(dx, dy);
	return false;
    }

    // Move in general direction, preferring a straight line.
    angle = rand_dirtoangle(dx, dy);

    for (j = 0; j <= 2; j++)
    {
	// To flank, we prefer the non-straight approach.
	switch (j)
	{
	    case 0:
		i = 1;
		break;
	    case 1:
		i = 0;
		break;
	    case 2:
		i = 2;
		break;
	}
	rand_angletodir(angle-i, dx, dy);
	if (canMoveDir(dx, dy))
	{
	    choice++;
	    if (!rand_choice(choice))
		resultangle = angle-i;
	}
	
	rand_angletodir(angle+i, dx, dy);
	if (canMoveDir(dx, dy))
	{
	    choice++;
	    if (!rand_choice(choice))
		resultangle = angle+i;
	}

	if (choice)
	{
	    rand_angletodir(resultangle, dx, dy);
	    return actionWalk(dx, dy);
	}
    }

    return false;
}

