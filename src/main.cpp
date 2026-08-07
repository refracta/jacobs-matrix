/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        main.cpp ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#include <libtcod.hpp>
#include <stdio.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#include <process.h>
#define usleep(x) Sleep((x)*1000)
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")

// I really hate windows.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#else
#include <unistd.h>
#endif

#include <fstream>
using namespace std;

int		 glbLevelStartMS = -1;

#include "rand.h"
#include "gfxengine.h"
#include "config.h"

#include <SDL.h>
#include <SDL_mixer.h>

Mix_Music		*glbTunes = 0;
bool			 glbMusicActive = false;

void endOfMusic()
{
    glbMusicActive = false;
}

void
stopMusic()
{
    if (glbMusicActive)
    {
	Mix_HaltMusic();
	glbMusicActive = false;
    }
}

void
startMusic()
{
    if (glbMusicActive)
	stopMusic();

    if (!glbTunes)
	glbTunes = Mix_LoadMUS(glbConfig->musicFile());
    if (!glbTunes)
    {
	printf("Failed to load %s, error %s\n", 
		glbConfig->musicFile(), 
		Mix_GetError());
    }
    else
    {
	glbMusicActive = true;
	glbLevelStartMS = TCOD_sys_elapsed_milli();

	if (glbConfig->musicEnable())
	{
	    Mix_PlayMusic(glbTunes, 0);
	    Mix_HookMusicFinished(endOfMusic);
	}
    }
}


#ifdef main
#undef main
#endif

#include "mob.h"
#include "map.h"
#include "text.h"
#include "speed.h"
#include "display.h"
#include "engine.h"
#include "panel.h"
#include "msg.h"
#include "fire.h"
#include "item.h"
#include "chooser.h"

PANEL		*glbMessagePanel = 0;
DISPLAY		*glbDisplay = 0;
MAP		*glbMap = 0;
ENGINE		*glbEngine = 0;
PANEL		*glbPopUp = 0;
PANEL		*glbJacob = 0;
PANEL		*glbWeaponInfo = 0;
PANEL		*glbVictoryInfo = 0;
PANEL		*glbDeathInfo = 0;
int		 glbDeathTS = -1;
int		 glbLastManaTS = -1;
int		 glbDeathCount = 0, glbRebootCount = 0;
bool		 glbPopUpActive = false;
bool		 glbRoleActive = false;
FIRE		*glbHealthFire = 0;
FIRE		*glbManaFire = 0;
int		 glbLevel = 1;
int		 glbMaxLevel = 0;
PTRLIST<int>	 glbLevelTimes;
bool		 glbVeryFirstRun = true;

CHOOSER		*glbRoleChooser = 0;
CHOOSER		*glbRoleStats = 0;

CHOOSER		*glbLevelChooser = 0;
bool		 glbLevelActive = false;

int		 glbInvertX = -1, glbInvertY = -1;

#define DEATHTIME 15000
#define MANATIME 1000


BUF
mstotime(int ms)
{
    int		sec, min;
    BUF		buf;

    sec = ms / 1000;
    ms -= sec * 1000;
    min = sec / 60;
    sec -= min * 60;

    if (min)
	buf.sprintf("%dm%d.%03ds", min, sec, ms);
    else if (sec)
	buf.sprintf("%d.%03ds", sec, ms);
    else
	buf.sprintf("0.%03ds", ms);

    return buf;
}

// Should call this in all of our loops.
void
redrawWorld()
{
    static MAP	*centermap = 0;
    static POS	 mapcenter;
    bool	 isblind = false;
    bool	 isvictory = false;

    gfx_updatepulsetime();

    if (glbMap)
	glbMap->decRef();
    glbMap = glbEngine->copyMap();

    glbWeaponInfo->clear();

    if (glbMap && glbMap->avatar())
    {
	MOB		*avatar = glbMap->avatar();
	int		 timems;

	timems = TCOD_sys_elapsed_milli();

	if (!avatar->alive())
	{
	    glbLastManaTS = -1;
	    // Check to see if a net new death.
	    if (avatar->numDeaths() > glbDeathCount)
	    {
		glbDeathCount = avatar->numDeaths();
		glbDeathTS = TCOD_sys_elapsed_milli();
	    }

	    // Is it time to reboot?
	    if (timems > glbDeathTS + DEATHTIME)
	    {
		if (glbRebootCount < glbDeathCount)
		{
		    glbEngine->queue().append(COMMAND(ACTION_REBOOTAVATAR));
		    glbRebootCount = glbDeathCount;
		}
	    }
	}
	else
	{
	    glbDeathTS = -1;
	    if (glbLastManaTS < 0)
	    {
		glbLastManaTS = timems;
		glbLastManaTS -= glbLastManaTS % MANATIME;
	    }

	    int		mana;

	    mana = timems - glbLastManaTS;
	    mana /= MANATIME;
	    glbLastManaTS += mana * MANATIME;

	    if (mana)
		glbEngine->queue().append(COMMAND(ACTION_MANAPULSE, mana));
	}

	if (centermap)
	    centermap->decRef();
	centermap = glbMap;
	centermap->incRef();
	mapcenter = avatar->pos();

	isblind = avatar->hasItem(ITEM_BLIND);

	if (avatar->hasItem(ITEM_INVULNERABLE))
	    glbHealthFire->setFireType(FIRE_MONO);
	else
	    glbHealthFire->setFireType(FIRE_BLACKBODY);

	glbHealthFire->setFlameSize(avatar->getHP() / (float) avatar->defn().max_hp);
	glbManaFire->setFlameSize(avatar->getMP() / (float) avatar->defn().max_mp);

	BUF		buf;
	ITEM		*item;

	item = avatar->lookupItem(ITEM_WEAPON);
	glbWeaponInfo->setTextAttr(ATTR_WHITE);
	if (item)
	{
	    glbWeaponInfo->appendText(gram_capitalize(item->getName()));
	    glbWeaponInfo->newLine();
	    glbWeaponInfo->setTextAttr(ATTR_NORMAL);
	    glbWeaponInfo->appendText(item->getDetailedDescription());
	}
	else
	    glbWeaponInfo->appendText("No Weapon.\n");
	glbWeaponInfo->newLine();

	glbWeaponInfo->setTextAttr(ATTR_WHITE);
	item = avatar->lookupItem(ITEM_SPELLBOOK);
	if (item)
	{
	    glbWeaponInfo->appendText(gram_capitalize(item->getName()));
	    glbWeaponInfo->newLine();
	    glbWeaponInfo->setTextAttr(ATTR_NORMAL);
	    glbWeaponInfo->appendText(item->getDetailedDescription());
	}
	else
	    glbWeaponInfo->appendText("No Spellbook.");
	glbWeaponInfo->newLine();

	isvictory = avatar->pos().victoryPos();

	if (isvictory)
	{
	    glbVictoryInfo->clear();
	    glbVictoryInfo->appendText("Press 'v' for Victory!");
	}

	if (glbDeathTS >= 0)
	{
	    float		percent;

	    percent = (timems - glbDeathTS) / (float) DEATHTIME;
	    percent = BOUND(percent, 0.0F, 1.0F);

	    glbDeathInfo->appendText(" Dead, awaiting revival... ");
	    glbDeathInfo->fillRect(0, 0, (int)(percent * 30), 1, ATTR_DEATHBAR);
	}
    }
    
    glbDisplay->display(mapcenter, isblind);
    glbMessagePanel->redraw();

    glbJacob->clear();
    glbJacob->setTextAttr(ATTR_WHITE);
    BUF		buf;

    buf.sprintf(" Depth: %d\n\n", glbMap ? glbMap->getDepth() : 0);

    glbJacob->appendText(buf);
    glbJacob->appendText(" Jacobian:\n");
    glbJacob->setTextAttr(ATTR_NORMAL);
    int		*jacob;

    jacob = mapcenter.jacobian();

    buf.sprintf("%4d %4d\n", jacob[0], jacob[1]);
    glbJacob->appendText(buf);
    buf.sprintf("%4d %4d\n", jacob[2], jacob[3]);
    glbJacob->appendText(buf);

    glbJacob->redraw();

    glbWeaponInfo->redraw();

    if (glbDeathTS >= 0)
	glbDeathInfo->redraw();

    if (isvictory)
	glbVictoryInfo->redraw();

    {
	FIRETEX		*tex = glbHealthFire->getTex();
	tex->redraw(0, 0);
	tex->decRef();
    }
    {
	FIRETEX		*tex = glbManaFire->getTex();
	tex->redraw(60, 0);
	tex->decRef();
    }

    if (glbInvertX >= 0 && glbInvertY >= 0)
    {
	gfx_printattr(glbInvertX, glbInvertY, ATTR_HILITE);
    }

    if (glbRoleActive)
    {
	glbRoleChooser->redraw();
	glbRoleStats->redraw();
    }

    if (glbLevelActive)
    {
	glbLevelChooser->redraw();
    }

    if (glbPopUpActive)
	glbPopUp->redraw();

    TCODConsole::flush();
}

// Pops up the given text onto the screen.
// The key used to dismiss the text is the return code.
int
popupText(const char *buf, int delayms = 0)
{
    int		key = 0;
    int		startms = TCOD_sys_elapsed_milli();

    glbPopUp->clear();
    // Must make active first because append text may trigger a 
    // redraw if it is long!
    glbPopUpActive = true;
    glbPopUp->appendText(buf);

    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);
	if (key)
	{
	    // Don't let them abort too soon.
	    if (((int)TCOD_sys_elapsed_milli() - startms) >= delayms)
		break;
	}
    }

    glbPopUpActive = false;
    redrawWorld();

    return key;
}

int
popupText(BUF buf, int delayms = 0)
{
    return popupText(buf.buffer(), delayms);
}

void
buildRoleStats(ROLE_NAMES role)
{
    glbRoleStats->clear();

    BUF		buf, tmp;

    glbRoleStats->setTextAttr(ATTR_WHITE);
    tmp = gram_makeplural(gram_capitalize(glb_roledefs[role].name));
    buf.sprintf("%s Prefer:", tmp.buffer());
    glbRoleStats->appendChoice(buf);
    glbRoleStats->appendChoice("");
    glbRoleStats->appendChoice("Weapon:");
    glbRoleStats->setTextAttr(ATTR_NORMAL);

    glbRoleStats->appendChoice("Power", glb_roledefs[role].w_power);
    glbRoleStats->appendChoice("Accuracy", glb_roledefs[role].w_accuracy);
    glbRoleStats->appendChoice("Consistency", glb_roledefs[role].w_consistency);

    glbRoleStats->setTextAttr(ATTR_WHITE);
    glbRoleStats->appendChoice("");
    glbRoleStats->appendChoice("Spellbook:");
    glbRoleStats->setTextAttr(ATTR_NORMAL);
    glbRoleStats->appendChoice("Range", glb_roledefs[role].b_range);
    glbRoleStats->appendChoice("Power", glb_roledefs[role].b_power);
    glbRoleStats->appendChoice("Consistency", glb_roledefs[role].b_consistency);
    glbRoleStats->appendChoice("Area", glb_roledefs[role].b_area);
    glbRoleStats->appendChoice("Low-Mana", glb_roledefs[role].b_mana);
}


// Selects a role.
int
selectRole()
{
    int		key;
    ROLE_NAMES	role;

    glbRoleChooser->clear();
    FOREACH_ROLE(role)
    {
	glbRoleChooser->appendChoice(gram_capitalize(glb_roledefs[role].name));
    }

    glbRoleChooser->setChoice(MOB::avatarRole());

    buildRoleStats((ROLE_NAMES) glbRoleChooser->getChoice());

    glbRoleActive = true;
    msg_report("Select a new role for yourself.  ");

    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);

	if (glbRoleChooser->processKey(key))
	{
	    buildRoleStats((ROLE_NAMES) glbRoleChooser->getChoice());
	}

	if (key)
	{
	    // User selects this?
	    if (key == '\n' || key == ' ')
	    {
		glbEngine->queue().append(COMMAND(ACTION_CHANGEROLE, glbRoleChooser->getChoice()));
		break;
	    }
	    else
	    {
		msg_report("Cancelled.  ");
		break;
	    }
	}
    }
    glbRoleActive = false;
    redrawWorld();

    return key;
}

//
// Freezes the game until a key is pressed.
// If key is not direction, false returned and it is eaten.
// else true returned with dx/dy set.  Can be 0,0.
bool
awaitDirection(int &dx, int &dy)
{
    int			key;

    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();

	key = gfx_getKey(false);

	if (gfx_cookDir(key, dx, dy))
	{
	    return true;
	}
	if (key)
	    return false;
    }

    return false;
}

void
doExamine()
{
    int		x = glbDisplay->x() + glbDisplay->width()/2;
    int		y = glbDisplay->y() + glbDisplay->height()/2;
    int		key;
    int		dx, dy;
    POS		p;
    BUF		buf;
    bool	blind;

    glbInvertX = x;
    glbInvertY = y;
    blind = false;
    if (glbMap && glbMap->avatar())
	blind = glbMap->avatar()->hasItem(ITEM_BLIND);
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();

	key = gfx_getKey(false);

	if (gfx_cookDir(key, dx, dy))
	{
	    // Space we want to select.
	    if (!dx && !dy)
		break;
	    x += dx;
	    y += dy;
	    x = BOUND(x, glbDisplay->x(), glbDisplay->x()+glbDisplay->width()-1);
	    y = BOUND(y, glbDisplay->y(), glbDisplay->y()+glbDisplay->height()-1);

	    glbInvertX = x;
	    glbInvertY = y;
	    p = glbDisplay->lookup(x, y);

	    if (p.tile() != TILE_INVALID && p.isFOV())
	    {
		p.describeSquare(blind);
	    }

	    msg_newturn();
	}

	// Any other key stops the look.
	if (key)
	    break;
    }

    // Check if we left on a mob.
    // Must re look up as displayWorld may have updated the map.
    p = glbDisplay->lookup(x, y);
    if (p.mob())
    {
	popupText(text_lookup("mob", p.mob()->getRawName()));
    }

    glbInvertX = -1;
    glbInvertY = -1;
}

void
reloadLevel(int depth)
{
    glbDeathCount = 0;
    glbRebootCount = 0;
    glbLastManaTS = -1;

    // Request a level from the user.
    glbLevelChooser->clear();
    int		i, key;
    BUF		buf;

    const char 	*levelnames[10] =
    {
	"Training Grounds ",
	"Easy             ",
	"Straight Forward ",
	"Hard             ",
	"Harder           ",
	"Difficult        ",
	"Impossible       ",
	"Implausible      ",
	"Out of Adjectives",
	"Final Level      "
    };

    for (i = 1; i <= 10; i++)
    {
	buf.clear();
	if (i <= glbMaxLevel)
	    glbLevelChooser->setTextAttr(ATTR_WHITE);
	else if (i == glbMaxLevel+1)
	    glbLevelChooser->setTextAttr(ATTR_NORMAL);
	else 
	    glbLevelChooser->setTextAttr(ATTR_OUTOFFOV);
	buf.strcat(levelnames[i-1]);
	if (i <= glbLevelTimes.entries() && glbLevelTimes(i-1) >= 0)
	{
	    buf.strcat(" (");
	    buf.strcat(mstotime(glbLevelTimes(i-1)));
	    buf.strcat(")");
	}
	glbLevelChooser->appendChoice(buf);
    }

    // Now for unlocked challenges..
    while (i <= glbMaxLevel+1)
    {
	buf.clear();
	if (i <= glbMaxLevel)
	    glbLevelChooser->setTextAttr(ATTR_RED);
	else if (i == glbMaxLevel+1)
	    glbLevelChooser->setTextAttr(ATTR_DKRED);
	buf.sprintf("Extreme %d       ", i);
	if (i < glbLevelTimes.entries() && glbLevelTimes(i-1) >= 0)
	{
	    buf.strcat(" (");
	    buf.strcat(mstotime(glbLevelTimes(i-1)));
	    buf.strcat(")");
	}
	glbLevelChooser->appendChoice(buf);
	i++;
    }

    glbLevelChooser->setChoice(depth-1);
    glbLevelActive = true;


    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);

	glbLevelChooser->processKey(key);

	if (key)
	{
	    // User selects this?
	    if (key == '\n' || key == ' ')
	    {
		// Ensure valid.
		if (glbLevelChooser->getChoice() <= glbMaxLevel)
		{
		    depth = glbLevelChooser->getChoice()+1;
		    break;
		}
	    }
	    else
	    {
		// Ignore other options.
	    }
	}
    }
    glbLevelActive = false;

    if (TCODConsole::isWindowClosed())
	return;

    glbVeryFirstRun = false;

    glbLevel = depth;
    glbEngine->queue().append(COMMAND(ACTION_RESTART, glbLevel));

    BUF		victitle;
    victitle.sprintf("You%d", (depth < 11) ? depth : 11);
    popupText(text_lookup("welcome", victitle));

    startMusic();
}

void
saveWorld()
{
    // Did I mention my hatred of streams?
#ifdef WIN32
    ofstream	os("jacob.sav", ios::out | ios::binary);
#else
    ofstream	os("jacob.sav");
#endif

    int32		val;
    int			i;
    u8			c;

    // What we unlocked
    val = glbMaxLevel;
    os.write((const char *) &val, sizeof(int32));

    // The level times scored
    val = glbLevelTimes.entries();
    os.write((const char *) &val, sizeof(int32));
    for (i = 0; i < glbLevelTimes.entries(); i++)
    {
	val = glbLevelTimes(i);
	os.write((const char *) &val, sizeof(int32));
    }

    val = MOB::avatarRole();
    os.write((const char *) &val, sizeof(int32));

    // Save out the avatar, if present.
    if (glbMap && glbMap->avatar())
    {
	c = 1;
	os.write((const char *) &c, 1);
	glbMap->avatar()->save(os);
    }
    else
    {
	c = 0;
	os.write((const char *) &c, 1);
    }
}

MOB *
loadWorld()
{
    // Open file for reading.
#ifdef WIN32
    ifstream	is("jacob.sav", ios::in | ios::binary);
#else
    ifstream	is("jacob.sav");
#endif

    if (!is)
	return 0;

    MOB		*avatar = 0;

    int32	 val;
    u8		 c;
    int		 i, n;

    is.read((char *) &val, sizeof(int32));
    glbMaxLevel = val;
    
    is.read((char *) &val, sizeof(int32));
    n = val;

    glbLevelTimes.clear();
    for (i = 0; i < n; i++)
    {
	is.read((char *) &val, sizeof(int32));
	glbLevelTimes.append(val);
    }

    is.read((char *) &val, sizeof(int32));
    MOB::setAvatarRole((ROLE_NAMES) val);

    is.read((char *) &c, 1);

    if (c)
    {
	avatar = MOB::load(is);
	// Saved positions are irrelevant here.
	avatar->clearAllPos();
    }

    return avatar;
}


int 
main(int argc, char **argv)
{
    bool		done = false;

    // Cannot be bool!
    // libtcod does not accept bools properly.
    int			fullscreen = false;

    glbConfig = new CONFIG();
    glbConfig->load("../jacob.cfg");

    fullscreen = glbConfig->screenFull();

    // Dear Microsoft,
    // The following code in optimized is both a waste and seems designed
    // to ensure that if we call a bool where the callee pretends it is
    // an int we'll end up with garbage data.
    //
    // 0040BF4C  movzx       eax,byte ptr [eax+10h] 

    // TCODConsole::initRoot(SCR_WIDTH, SCR_HEIGHT, "Jacob's Matrix", fullscreen);
    // 0040BF50  xor         ebp,ebp 
    // 0040BF52  cmp         eax,ebp 
    // 0040BF54  setne       cl   
    // 0040BF57  mov         dword ptr [esp+28h],eax 
    // 0040BF5B  push        ecx  

    // My work around is to constantify the fullscreen and hope that
    // the compiler doesn't catch on.
    if (fullscreen)
	TCODConsole::initRoot(SCR_WIDTH, SCR_HEIGHT, "Jacob's Matrix", true);
    else
	TCODConsole::initRoot(SCR_WIDTH, SCR_HEIGHT, "Jacob's Matrix", false);

    // TCOD doesn't do audio.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (Mix_OpenAudio(22050, AUDIO_S16, 2, 4096))
    {
	printf("Failed to open audio!\n");
	exit(1);
    }

    rand_setseed((long) time(0));

    gfx_init();
    MAP::init();

    text_init();
    spd_init();

    MOB::setAvatarRole(ROLE_TOURIST);

    glbDisplay = new DISPLAY(0, 0, 80, 40);

    glbMessagePanel = new PANEL(40, 10);
    msg_registerpanel(glbMessagePanel);
    glbMessagePanel->move(20, 40);

    glbPopUp = new PANEL(50, 30);
    glbPopUp->move(15, 10);
    glbPopUp->setBorder(true, ' ', ATTR_BORDER);
    // And so the mispelling is propagated...
    glbPopUp->setRigthMargin(1);
    glbPopUp->setIndent(1);

    glbJacob = new PANEL(11, 6);
    glbJacob->move(65, 1);

    glbWeaponInfo = new PANEL(20, 16);
    glbWeaponInfo->move(1, 1);

    glbVictoryInfo = new PANEL(26, 1);
    glbVictoryInfo->move(28, 7);
    glbVictoryInfo->setIndent(2);
    glbVictoryInfo->setBorder(true, ' ', ATTR_VICTORYBORDER);

    glbDeathInfo = new PANEL(30, 1);
    glbDeathInfo->move(25, 10);
    glbDeathInfo->setBorder(true, ' ', ATTR_DEATHBORDER);
    glbDeathInfo->setIndent(2);
    glbDeathTS = -1;
    glbLastManaTS = -1;
    glbDeathCount = 0;
    glbRebootCount = 0;
    glbLevel = 1;

    glbRoleChooser = new CHOOSER();
    glbRoleChooser->move(30, 25, CHOOSER::JUSTRIGHT, CHOOSER::JUSTCENTER);
    glbRoleChooser->setBorder(true, ' ', ATTR_BORDER);
    glbRoleChooser->setIndent(1);
    glbRoleChooser->setRigthMargin(1);

    glbRoleStats = new CHOOSER();
    glbRoleStats->move(31, 25, CHOOSER::JUSTLEFT, CHOOSER::JUSTCENTER);
    glbRoleStats->setBorder(true, ' ', ATTR_BORDER);
    glbRoleStats->setMinW(20);
    glbRoleStats->setAttr(ATTR_NORMAL, ATTR_HILITE, ATTR_POWERBAR);

    glbLevelChooser = new CHOOSER();
    glbLevelChooser->move(40, 25, CHOOSER::JUSTCENTER, CHOOSER::JUSTCENTER);
    glbLevelChooser->setBorder(true, ' ', ATTR_BORDER);
    glbLevelChooser->setIndent(1);
    glbLevelChooser->setRigthMargin(1);


    {
	MOB		*avatar;

	avatar = loadWorld();
	glbEngine = new ENGINE(glbDisplay, avatar);
    }

    glbHealthFire = new FIRE(glbConfig->flameWidth(), glbConfig->flameHeight(), 20, 50, FIRE_BLACKBODY);
    glbManaFire = new FIRE(glbConfig->flameWidth(), glbConfig->flameHeight(), 20, 50, FIRE_ICE);

    reloadLevel(1);

    do
    {
	int		key;
	int		dx, dy;

	redrawWorld();

	if (glbMap && glbMap->avatar())
	{
	    if (!glbMap->avatar()->pos().victoryPos())
	    {
		if (!glbMusicActive)
		{
		    // Ensure the avatar is dead...
		    glbEngine->queue().append(COMMAND(ACTION_SUICIDE));
		    // Out of time!
		    // There is a delay to restart since otherwise
		    // they likely are hammering the keyboard at this point
		    popupText(text_lookup("game", "lose"), 2000);
		    reloadLevel(glbLevel);
		}
	    }
	}

	key = gfx_getKey(false);

	if (gfx_cookDir(key, dx, dy))
	{
	    // Direction.
	    glbEngine->queue().append(COMMAND(ACTION_BUMP, dx, dy));
	}

	switch (key)
	{
	    case 'Q':
		done = true;
		break;

	    case 'R':
		reloadLevel(glbLevel);
		break;

	    case '/':
	    case 'e':
		glbEngine->queue().append(COMMAND(ACTION_ROTATE, 1));
		break;
	    case '*':
	    case 'r':
		glbEngine->queue().append(COMMAND(ACTION_ROTATE, 3));
		break;

	    case 'w':
	    case '\n':
		selectRole();
		break;

	    case 'D':
		glbEngine->queue().append(COMMAND(ACTION_DROP));
		break;

	    case '+':
	    case '=':
	    case '-':
	    case 't':
	    case 'g':
	    {
		int		portal = 0;
		BUF		buf;
		if (key == '+' || key == '=' || key == 'g')
		    portal = 1;

		buf.sprintf("Cast %s portal in what direction?  ", portal ? "orange" : "blue");

		msg_report(buf);
		if (awaitDirection(dx, dy) && (dx || dy))
		{
		    msg_report(rand_dirtoname(dx, dy));
		    msg_newturn();
		    glbEngine->queue().append(COMMAND(ACTION_FIRE, dx, dy, portal+1));
		}
		else
		{
		    msg_report("Cancelled.  ");
		    msg_newturn();
		}
		break;
	    }

	    case 'f':
	    case '0':
		msg_report("Cast spell in what direction?  ");
		if (awaitDirection(dx, dy) && (dx || dy))
		{
		    msg_report(rand_dirtoname(dx, dy));
		    msg_newturn();
		    glbEngine->queue().append(COMMAND(ACTION_FIRE, dx, dy));
		}
		else
		{
		    msg_report("Cancelled.  ");
		    msg_newturn();
		}
		break;

	    case 'a':
		popupText(text_lookup("game", "about"));
		break;

	    case 'z':
	    {
		BUF		victitle;
		int		depth = glbMap ? glbMap->getDepth() : 1;
		victitle.sprintf("You%d", (depth < 11) ? depth : 11);
		popupText(text_lookup("welcome", victitle));
		break;
	    }

	    case 'P':
		fullscreen = !fullscreen;
		// This is intentionally unrolled to work around a
		// bool/int problem in libtcod
		if (fullscreen)
		    TCODConsole::setFullscreen(true);
		else
		    TCODConsole::setFullscreen(false);
		break;
	
	    case GFX_KEYF1:
	    case '?':
		popupText(text_lookup("game", "help"));
		break;

	    case 'v':
		if (glbMap && glbMap->avatar())
		{
		    if (glbMap->avatar()->alive() &&
			glbMap->avatar()->pos().victoryPos())
		    {
			int		victorytime = TCOD_sys_elapsed_milli();
			int		oldtime;
			BUF		victitle;
			BUF		maintext;
			int		depth = glbMap->getDepth();

			victitle.sprintf("victory%d", (depth < 11) ? depth : 11);

			victorytime -= glbLevelStartMS;
			maintext.strcat("Your time: ");
			maintext.strcat(mstotime(victorytime));
			maintext.strcat("\n");
			while (glbLevelTimes.entries() < depth)
			    glbLevelTimes.append(-1);
			oldtime = glbLevelTimes(depth-1);
			if (oldtime < 0)
			{
			    // Invalid old time, new best.
			    maintext.strcat("This is now the time to beat for this level.\n");
			    glbLevelTimes.set(depth-1, victorytime);
			}
			else if (oldtime < victorytime)
			{
			    maintext.strcat("You did not beat the previous best of ");
			    maintext.strcat(mstotime(oldtime));
			    maintext.strcat(", you fell short by ");
			    maintext.strcat(mstotime(victorytime-oldtime));
			    maintext.strcat(".\n");
			}
			else
			{
			    // New record!
			    maintext.strcat("You beat the previous best of ");
			    maintext.strcat(mstotime(oldtime));
			    maintext.strcat(" by ");
			    maintext.strcat(mstotime(oldtime-victorytime));
			    maintext.strcat(".\n");

			    glbLevelTimes.set(depth-1, victorytime);
			}

			maintext.strcat("\n");
			maintext.strcat(text_lookup("game", victitle));
			popupText(maintext);

			if (glbMap->getDepth() > glbMaxLevel)
			    glbMaxLevel = glbMap->getDepth();

			reloadLevel(glbMap->getDepth()+1);
		    }
		    else if (glbMap->avatar()->pos().victoryPos())
		    {
			msg_report("The dead cannot declare victory.  ");
		    }
		    else
		    {
			msg_report("Reach the gold room first!  ");
		    }
		}
		break;

	    case 'x':
		doExamine();
		break;
	}
    } while (!done && !TCODConsole::isWindowClosed());

    stopMusic();
    if (glbTunes)
	Mix_FreeMusic(glbTunes);

    if (!glbVeryFirstRun)
	saveWorld();

    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    SDL_Quit();

    return 0;
}
