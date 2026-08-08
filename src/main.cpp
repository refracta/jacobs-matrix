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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define JACOB_SAVE_PATH "/save/jacob.sav"
#else
#define JACOB_SAVE_PATH "jacob.sav"
#endif

int		 glbLevelStartMS = -1;

#include "rand.h"
#include "gfxengine.h"
#include "config.h"
#include "language.h"

#include <SDL.h>
#include <SDL_mixer.h>

Mix_Music		*glbTunes = 0;
bool			 glbMusicActive = false;

// Cannot be bool!
// libtcod does not accept bools properly.
int			glbFullScreen = false;
int			glbMusicVolume = 10;

void endOfMusic()
{
    glbMusicActive = false;
}

void
setMusicVolume(int volume)
{
    volume = BOUND(volume, 0, 10);

    glbMusicVolume = volume;

    Mix_VolumeMusic((MIX_MAX_VOLUME * volume) / 10);
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
    int playResult = -2;

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
	    playResult = Mix_PlayMusic(glbTunes, 0);
	    if (playResult)
	    {
		printf("Failed to play music, error %s\n", Mix_GetError());
		glbMusicActive = false;
	    }
	    Mix_HookMusicFinished(endOfMusic);

	    setMusicVolume(glbMusicVolume);
	}
    }

#ifdef __EMSCRIPTEN__
    MAIN_THREAD_EM_ASM({
	globalThis.__jacobMusicStatus = {};
	globalThis.__jacobMusicStatus.loaded = !!$0;
	globalThis.__jacobMusicStatus.enabled = !!$1;
	globalThis.__jacobMusicStatus.playResult = $2;
    }, glbTunes != 0, glbConfig->musicEnable(), playResult);
#endif
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
bool		 glbDrawMap = false;

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

    if (language_is_korean() && min)
	buf.sprintf("%d분 %d.%03d초", min, sec, ms);
    else if (language_is_korean())
	buf.sprintf("%d.%03d초", sec, ms);
    else if (min)
	buf.sprintf("%dm%d.%03ds", min, sec, ms);
    else if (sec)
	buf.sprintf("%d.%03ds", sec, ms);
    else
	buf.sprintf("0.%03ds", ms);

    return buf;
}

void
drawMap(MAP *map)
{
    if (!map) return;

    MOB 	*avatar = map->avatar();

    if (!avatar)
	return;
    
    POS		pos = avatar->pos();
    int		rw, rh;

    rw = map->getDepth() + 2;
    rh = map->getDepth() + 2;

    int		rx = pos.roomId() % rw;
    int		ry = pos.roomId() / rw;

    int		x, y;

    // Note that we flip x/y when we render this map.
    // This is because we made the first column of the jacobian
    // dr, the second dg.  The first column in the map should be the
    // x axis and second y axis, but we vary red by ry, not green, so
    // we have to swap to match expectations.
    // 
    // Likewise, note that +y is down, this conforms to the game's space
    // with the jacobian where +y is down.

    for (y = -10; y <= 10; y++)
    {
	for (x = -10; x <= 10; x++)
	{
	    if (ABS(x) == 10 || ABS(y) == 10)
	    {
		// Border.
		gfx_printchar(40 + x, 25 + y, ' ',
				0, 0, 0,
				200, 160, 80);
	    }
	    else if (rx + x < 0 || rx + x >= rw ||
		     ry + y < 0 || ry + y >= rh)
	    {
		// Out of bounds.
		gfx_printchar(40 + y, 25 + x, ' ',
				0, 0, 0);
	    }
	    else if (x == 0 && y == 0)
	    {
		gfx_printchar(40 + y, 25 + x, '#',
				(u8) (((ry+y) / (float)(rh-1)) * 255),
				(u8) (((rx+x) / (float)(rw-1)) * 255),
				(u8) (255 - 255*((rx+x+ry+y) / (float)(rw+rh-2))),
				255, 255, 255);
	    }
	    else
	    {
		gfx_printchar(40 + y, 25 + x, '#',
				(u8) (((ry+y) / (float)(rh-1)) * 255),
				(u8) (((rx+x) / (float)(rw-1)) * 255),
				(u8) (255 - 255*((rx+x+ry+y) / (float)(rw+rh-2))));
	    }
	}
    }
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
	    glbWeaponInfo->appendText(language_is_korean() ? item->getName() :
				gram_capitalize(item->getName()));
	    glbWeaponInfo->newLine();
	    glbWeaponInfo->setTextAttr(ATTR_NORMAL);
	    glbWeaponInfo->appendText(item->getDetailedDescription());
	}
	else
	    glbWeaponInfo->appendText(language_text("No Weapon.\n"));
	glbWeaponInfo->newLine();

	glbWeaponInfo->setTextAttr(ATTR_WHITE);
	item = avatar->lookupItem(ITEM_SPELLBOOK);
	if (item)
	{
	    glbWeaponInfo->appendText(language_is_korean() ? item->getName() :
				gram_capitalize(item->getName()));
	    glbWeaponInfo->newLine();
	    glbWeaponInfo->setTextAttr(ATTR_NORMAL);
	    glbWeaponInfo->appendText(item->getDetailedDescription());
	}
	else
	    glbWeaponInfo->appendText(language_text("No Spellbook."));
	glbWeaponInfo->newLine();

	isvictory = avatar->pos().victoryPos();

	if (isvictory)
	{
	    glbVictoryInfo->clear();
	    glbVictoryInfo->appendText(language_text("Press 'v' for Victory!"));
	}

	if (glbDeathTS >= 0)
	{
	    float		percent;

	    percent = (timems - glbDeathTS) / (float) DEATHTIME;
	    percent = BOUND(percent, 0.0F, 1.0F);

	    // This panel is redrawn every frame.  Without clearing it first,
	    // multi-byte text is repeatedly appended and appears to jump/flicker.
	    glbDeathInfo->clear();
	    glbDeathInfo->fillRect(0, 0, 30, 1, ATTR_NORMAL);
	    glbDeathInfo->appendText(language_text(" Dead, awaiting revival... "));
	    glbDeathInfo->fillRect(0, 0, (int)(percent * 30), 1, ATTR_DEATHBAR);
	}
    }
    
    glbDisplay->display(mapcenter, isblind);
    glbMessagePanel->redraw();

    glbJacob->clear();
    glbJacob->setTextAttr(ATTR_WHITE);
    BUF		buf;

    if (language_is_korean())
	buf.sprintf(" 깊이: %d\n\n", glbMap ? glbMap->getDepth() : 0);
    else
	buf.sprintf(" Depth: %d\n\n", glbMap ? glbMap->getDepth() : 0);

    glbJacob->appendText(buf);
    glbJacob->appendText(language_text(" Jacobian:\n"));
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

    if (glbDrawMap)
	drawMap(glbMap);

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

    if (glbLevelActive)
    {
	glbLevelChooser->redraw();
    }

    if (glbRoleActive)
    {
	glbRoleChooser->redraw();
	glbRoleStats->redraw();
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
    if (language_is_korean())
    {
	tmp.reference(language_text(glb_roledefs[role].name));
	buf.sprintf("%s 선호:", tmp.buffer());
    }
    else
    {
	tmp = gram_makeplural(gram_capitalize(glb_roledefs[role].name));
	buf.sprintf("%s Prefer:", tmp.buffer());
    }
    glbRoleStats->appendChoice(buf);
    glbRoleStats->appendChoice("");
    glbRoleStats->appendChoice(language_text("Weapon:"));
    glbRoleStats->setTextAttr(ATTR_NORMAL);

    glbRoleStats->appendChoice(language_text("Power"), glb_roledefs[role].w_power);
    glbRoleStats->appendChoice(language_text("Accuracy"), glb_roledefs[role].w_accuracy);
    glbRoleStats->appendChoice(language_text("Consistency"), glb_roledefs[role].w_consistency);

    glbRoleStats->setTextAttr(ATTR_WHITE);
    glbRoleStats->appendChoice("");
    glbRoleStats->appendChoice(language_text("Spellbook:"));
    glbRoleStats->setTextAttr(ATTR_NORMAL);
    glbRoleStats->appendChoice(language_text("Range"), glb_roledefs[role].b_range);
    glbRoleStats->appendChoice(language_text("Power"), glb_roledefs[role].b_power);
    glbRoleStats->appendChoice(language_text("Consistency"), glb_roledefs[role].b_consistency);
    glbRoleStats->appendChoice(language_text("Area"), glb_roledefs[role].b_area);
    glbRoleStats->appendChoice(language_text("Low-Mana"), glb_roledefs[role].b_mana);
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
	if (language_is_korean())
	    glbRoleChooser->appendChoice(language_text(glb_roledefs[role].name));
	else
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
buildOptionsMenu(OPTION_NAMES d)
{
    OPTION_NAMES	option;
    glbLevelChooser->clear();

    glbLevelChooser->setTextAttr(ATTR_NORMAL);
    FOREACH_OPTION(option)
    {
	glbLevelChooser->appendChoice(language_text(glb_optiondefs[option].name));
    }
    glbLevelChooser->setChoice(d);

    glbLevelActive = true;
}

bool
optionsMenu()
{
    int			key;
    bool		done = false;

    buildOptionsMenu(OPTION_INSTRUCTIONS);

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
		if (glbLevelChooser->getChoice() == OPTION_INSTRUCTIONS)
		{
		    // Instructions.
		    popupText(text_lookup("game", "help"));
		}
		else if (glbLevelChooser->getChoice() == OPTION_PLAY)
		{
		    // Play...
		    break;
		}
		else if (glbLevelChooser->getChoice() == OPTION_LANGUAGE)
		{
		    glbLevelChooser->clear();
		    glbLevelChooser->appendChoice("English");
		    glbLevelChooser->appendChoice("한국어");
		    glbLevelChooser->setChoice((int) language_get());

		    while (!TCODConsole::isWindowClosed())
		    {
			redrawWorld();
			key = gfx_getKey(false);
			if (glbLevelChooser->processKey(key))
			    language_set((LANGUAGE_NAMES) glbLevelChooser->getChoice());

			if (key == '\n' || key == ' ' || key == '\x1b')
			{
			    language_set((LANGUAGE_NAMES) glbLevelChooser->getChoice());
			    break;
			}
		    }
		    buildOptionsMenu(OPTION_LANGUAGE);
		}
		else if (glbLevelChooser->getChoice() == OPTION_VOLUME)
		{
		    int			i;
		    BUF			buf;
		    // Volume
		    glbLevelChooser->clear();

		    for (i = 0; i <= 10; i++)
		    {
			if (language_is_korean())
			    buf.sprintf("%3d%% 음량", (10 - i) * 10);
			else
			    buf.sprintf("%3d%% Volume", (10 - i) * 10);

			glbLevelChooser->appendChoice(buf);
		    }
		    glbLevelChooser->setChoice(10 - glbMusicVolume);

		    while (!TCODConsole::isWindowClosed())
		    {
			redrawWorld();
			key = gfx_getKey(false);

			glbLevelChooser->processKey(key);

			setMusicVolume(10 - glbLevelChooser->getChoice());
			if (key)
			{
			    break;
			}
		    }
		    buildOptionsMenu(OPTION_VOLUME);
		}
		else if (glbLevelChooser->getChoice() == OPTION_FLAMEQUALITY)
		{
		    // Flame Quality
		    int			i;
		    BUF			buf;
		    const char 		*quality[5] =
		    {
			"No Flames",
			"Low",
			"Middle",
			"High",
			"Silly"
		    };

		    int			widths[5] =
		    {	0,
			20,
			40,
			80,
			160
		    };
		    int			heights[5] =
		    {	0,
			50,
			100,
			200,
			400
		    };

		    // Flame Quality
		    glbLevelChooser->clear();

		    for (i = 0; i < 5; i++)
		    {
			glbLevelChooser->appendChoice(language_text(quality[i]));
		    }

		    for (i = 0; i < 5; i++)
		    {
			if (glbHealthFire->width() < widths[i])
			    break;
		    }
		    i--;
		    i = BOUND(i, 0, 5);
		    glbLevelChooser->setChoice(i);

		    while (!TCODConsole::isWindowClosed())
		    {
			bool	waskey = false;
			redrawWorld();
			key = gfx_getKey(false);

			if (key)
			    waskey = true;

			glbLevelChooser->processKey(key);

			if (key)
			{
			    break;
			}
			else if (waskey)
			{
			    // New selection likely, so update
			    glbHealthFire->resize(widths[glbLevelChooser->getChoice()], heights[glbLevelChooser->getChoice()]);
			    glbManaFire->resize(widths[glbLevelChooser->getChoice()], heights[glbLevelChooser->getChoice()]);
			}
		    }
		    buildOptionsMenu(OPTION_FLAMEQUALITY);
		}
		else if (glbLevelChooser->getChoice() == OPTION_CURRENTROLE)
		{
		    // Role
		    selectRole();
		}
		else if (glbLevelChooser->getChoice() == OPTION_FULLSCREEN)
		{
		    glbFullScreen = !glbFullScreen;
		    // This is intentionally unrolled to work around a
		    // bool/int problem in libtcod
		    if (glbFullScreen)
			TCODConsole::setFullscreen(true);
		    else
			TCODConsole::setFullscreen(false);
		}
		else if (glbLevelChooser->getChoice() == OPTION_QUIT)
		{
		    // Quit
		    done = true;
		    break;
		}
	    }
	    else if (key == '\x1b')
	    {
		// Esc on options is to go back to play.
		// Play...
		break;
	    }
	    else
	    {
		// Ignore other options.
	    }
	}
    }
    glbLevelActive = false;

    return done;
}

void
buildLevelChooser(int depth)
{
    // Request a level from the user.
    glbLevelChooser->clear();
    int		i;
    BUF		buf;

    const char 	*levelnames[10] =
    {
	"Training Grounds",
	"Easy",
	"Straight Forward",
	"Hard",
	"Harder",
	"Difficult",
	"Impossible",
	"Implausible",
	"Out of Adjectives",
	"Final Level"
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
	buf.strcat(language_text(levelnames[i-1]));
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
	if (language_is_korean())
	    buf.sprintf("극한 %d       ", i);
	else
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
}


bool
reloadLevel(int depth)
{
    int			key;

    glbDeathCount = 0;
    glbRebootCount = 0;
    glbLastManaTS = -1;

    buildLevelChooser(depth);
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
	    else if (key == '\x1b')
	    {
		depth = glbLevelChooser->getChoice()+1;
		// Esc on level choosing should bring us to Options.
		if (optionsMenu())
		{
		    return true;
		}

		// Otherwise continue, but we have to rebuild
		// as options dirtied the level chooser.
		buildLevelChooser(depth);
	    }
	    else
	    {
		// Ignore other options.
	    }
	}
    }
    glbLevelActive = false;

    if (TCODConsole::isWindowClosed())
	return true;

    glbVeryFirstRun = false;

    glbLevel = depth;
    glbEngine->queue().append(COMMAND(ACTION_RESTART, glbLevel));

    BUF		victitle;
    victitle.sprintf("You%d", (depth < 11) ? depth : 11);
    popupText(text_lookup("welcome", victitle));

    startMusic();

    return false;
}

void
saveWorld()
{
    // Did I mention my hatred of streams?
#ifdef WIN32
    ofstream	os(JACOB_SAVE_PATH, ios::out | ios::binary);
#else
    ofstream	os(JACOB_SAVE_PATH);
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

#ifdef __EMSCRIPTEN__
    os.close();
    EM_ASM({
	if (typeof FS !== "undefined" && FS.syncfs) {
	    FS.syncfs(false, function(error) {
		if (error) console.warn("Could not persist the saved game", error);
	    });
	}
    });
#endif
}

MOB *
loadWorld()
{
    // Open file for reading.
#ifdef WIN32
    ifstream	is(JACOB_SAVE_PATH, ios::in | ios::binary);
#else
    ifstream	is(JACOB_SAVE_PATH);
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


#ifdef WIN32
int WINAPI
WinMain(HINSTANCE hINstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCMdSHow)
#else
int 
main(int argc, char **argv)
#endif
{
    bool		done = false;

    glbConfig = new CONFIG();
#ifdef __EMSCRIPTEN__
    glbConfig->load("/jacob.cfg");
#else
    glbConfig->load("../jacob.cfg");
#endif

    glbFullScreen = glbConfig->screenFull();

#ifdef __EMSCRIPTEN__
    // Browser fullscreen can only be entered in response to a user gesture.
    glbFullScreen = false;
    // Emscripten's SDL 1 compatibility surfaces are RGBA.  Treat the bundled
    // black-and-white font as greyscale so libtcod builds a proper alpha mask.
    TCODConsole::setCustomFont("terminal.png", 8, 8,
	TCOD_FONT_LAYOUT_ASCII_INCOL | TCOD_FONT_TYPE_GREYSCALE);
#endif

    // Dear Microsoft,
    // The following code in optimized is both a waste and seems designed
    // to ensure that if we call a bool where the callee pretends it is
    // an int we'll end up with garbage data.
    //
    // 0040BF4C  movzx       eax,byte ptr [eax+10h] 

    // TCODConsole::initRoot(SCR_WIDTH, SCR_HEIGHT, "Jacob's Matrix", glbFullScreen);
    // 0040BF50  xor         ebp,ebp 
    // 0040BF52  cmp         eax,ebp 
    // 0040BF54  setne       cl   
    // 0040BF57  mov         dword ptr [esp+28h],eax 
    // 0040BF5B  push        ecx  

    // My work around is to constantify the fullscreen and hope that
    // the compiler doesn't catch on.
    if (glbFullScreen)
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

    setMusicVolume(glbConfig->musicVolume());

    rand_setseed((long) time(0));

    gfx_init();
    MAP::init();

    language_init();
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

    done = optionsMenu();
    if (done)
    {
	stopMusic();
	if (glbTunes)
	    Mix_FreeMusic(glbTunes);

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	SDL_Quit();
	return 0;
    }

    done = reloadLevel(1);
    if (done)
    {
	stopMusic();
	if (glbTunes)
	    Mix_FreeMusic(glbTunes);

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	SDL_Quit();
	return 0;
    }

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
		    done = reloadLevel(glbLevel);
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
		done = reloadLevel(glbLevel);
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

	    case 'm':
		glbDrawMap = !glbDrawMap;
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

		if (language_is_korean())
		    buf.sprintf("어느 방향에 %s 차원문을 만들까요?  ", portal ? "주황색" : "파란색");
		else
		    buf.sprintf("Cast %s portal in what direction?  ", portal ? "orange" : "blue");

		msg_report(buf);
		if (awaitDirection(dx, dy) && (dx || dy))
		{
		    msg_report(language_text(rand_dirtoname(dx, dy)));
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
		    msg_report(language_text(rand_dirtoname(dx, dy)));
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
		glbFullScreen = !glbFullScreen;
		// This is intentionally unrolled to work around a
		// bool/int problem in libtcod
		if (glbFullScreen)
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
			maintext.strcat(language_text("Your time: "));
			maintext.strcat(mstotime(victorytime));
			maintext.strcat("\n");
			while (glbLevelTimes.entries() < depth)
			    glbLevelTimes.append(-1);
			oldtime = glbLevelTimes(depth-1);
			if (oldtime < 0)
			{
			    // Invalid old time, new best.
			    maintext.strcat(language_text("This is now the time to beat for this level.\n"));
			    glbLevelTimes.set(depth-1, victorytime);
			}
			else if (oldtime < victorytime)
			{
			    maintext.strcat(language_text("You did not beat the previous best of "));
			    maintext.strcat(mstotime(oldtime));
			    maintext.strcat(language_text(", you fell short by "));
			    maintext.strcat(mstotime(victorytime-oldtime));
			    maintext.strcat(language_is_korean() ? " 늦었습니다.\n" : ".\n");
			}
			else
			{
			    // New record!
			    maintext.strcat(language_text("You beat the previous best of "));
			    maintext.strcat(mstotime(oldtime));
			    maintext.strcat(language_text(" by "));
			    maintext.strcat(mstotime(oldtime-victorytime));
			    maintext.strcat(language_is_korean() ? " 앞당겼습니다.\n" : ".\n");

			    glbLevelTimes.set(depth-1, victorytime);
			}

			maintext.strcat("\n");
			maintext.strcat(text_lookup("game", victitle));
			popupText(maintext);

			if (glbMap->getDepth() > glbMaxLevel)
			    glbMaxLevel = glbMap->getDepth();

			done = reloadLevel(glbMap->getDepth()+1);
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

	    case 'O':
	    case '\x1b':
		done = optionsMenu();
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
