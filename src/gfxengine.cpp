/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        gfxengine.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	This is a very light wrapper around libtcod.
 */

#include <libtcod.hpp>
#include "gfxengine.h"

TCODNoise		*glbPulseNoise = 0;
float			 glbPulseVal = 1.0f;

int
cookKey(TCOD_key_t key)
{
    switch (key.vk)
    {
	case TCODK_CHAR:
	    return key.c;

	case TCODK_ENTER:
	case TCODK_KPENTER:
	    return '\n';

	case TCODK_BACKSPACE:
	    return '\b';
	
	case TCODK_ESCAPE:
	    return '\x1b';

	case TCODK_TAB:
	    return '\t';

	case TCODK_SPACE:
	    return ' ';

	case TCODK_UP:
	    return GFX_KEYUP;
	case TCODK_DOWN:
	    return GFX_KEYDOWN;
	case TCODK_LEFT:
	    return GFX_KEYLEFT;
	case TCODK_RIGHT:
	    return GFX_KEYRIGHT;

	case TCODK_PAGEUP:
	    return GFX_KEYPAGEUP;
	case TCODK_PAGEDOWN:
	    return GFX_KEYPAGEDOWN;
	case TCODK_HOME:
	    return GFX_KEYHOME;
	case TCODK_END:
	    return GFX_KEYEND;

	case TCODK_F1:
	    return GFX_KEYF1;
	case TCODK_F2:
	    return GFX_KEYF2;
	case TCODK_F3:
	    return GFX_KEYF3;
	case TCODK_F4:
	    return GFX_KEYF4;
	case TCODK_F5:
	    return GFX_KEYF5;
	case TCODK_F6:
	    return GFX_KEYF6;
	case TCODK_F7:
	    return GFX_KEYF7;
	case TCODK_F8:
	    return GFX_KEYF8;
	case TCODK_F9:
	    return GFX_KEYF9;
	case TCODK_F10:
	    return GFX_KEYF10;
	case TCODK_F11:
	    return GFX_KEYF11;
	case TCODK_F12:
	    return GFX_KEYF12;

	case TCODK_0:
	case TCODK_1:
	case TCODK_2:
	case TCODK_3:
	case TCODK_4:
	case TCODK_5:
	case TCODK_6:
	case TCODK_7:
	case TCODK_8:
	case TCODK_9:
	    return '0' + key.vk - TCODK_0;

	case TCODK_KP0:
	case TCODK_KP1:
	case TCODK_KP2:
	case TCODK_KP3:
	case TCODK_KP4:
	case TCODK_KP5:
	case TCODK_KP6:
	case TCODK_KP7:
	case TCODK_KP8:
	case TCODK_KP9:
	    return '0' + key.vk - TCODK_KP0;

	case TCODK_KPADD:
	    return '+';
	case TCODK_KPSUB:
	    return '-';
	case TCODK_KPDIV:
	    return '/';
	case TCODK_KPMUL:
	    return '*';

	// To match up the keypad, hopefully.
	case TCODK_INSERT:
	    return '0';

	case TCODK_KPDEC:	// I think keypad del?
	    return '.';
    }

    return 0;
}

bool
gfx_cookDir(int &key, int &dx, int &dy, GFX_Cookdir cookdir)
{
    // Check diagonals only if both x & y.
    if ((cookdir & GFX_COOKDIR_X) && (cookdir & GFX_COOKDIR_Y))
    {
	switch (key)
	{
	    case GFX_KEYHOME:
	    case '7':
	    case 'y':
		dx = -1;	dy = -1;
		key = 0;
		return true;
	    case GFX_KEYPAGEUP:
	    case '9':
	    case 'u':
		dx = 1;	dy = -1;
		key = 0;
		return true;
	    case GFX_KEYEND:
	    case '1':
	    case 'b':
		dx = -1;	dy = 1;
		key = 0;
		return true;
	    case GFX_KEYPAGEDOWN:
	    case '3':
	    case 'n':
		dx = 1;	dy = 1;
		key = 0;
		return true;
	}
    }

    // Check left/right
    if (cookdir & GFX_COOKDIR_X)
    {
	switch (key)
	{
	    case GFX_KEYLEFT:
	    case '4':
	    case 'h':
		dx = -1;	dy = 0;
		key = 0;
		return true;
	    case GFX_KEYRIGHT:
	    case '6':
	    case 'l':
		dx = 1;	dy = 0;
		key = 0;
		return true;
	}
    }

    // Check up/down
    if (cookdir & GFX_COOKDIR_Y)
    {
	switch (key)
	{
	    case GFX_KEYDOWN:
	    case '2':
	    case 'j':
		dx = 0;	dy = 1;
		key = 0;
		return true;
	    case GFX_KEYUP:
	    case '8':
	    case 'k':
		dx = 0;	dy = -1;
		key = 0;
		return true;
	}
    }

    // Check stationary
    if (cookdir & GFX_COOKDIR_STATIONARY)
    {
	switch (key)
	{
	    case '5':
	    case '.':
	    case ' ':
		dx = 0;	dy = 0;
		key = 0;
		return true;
	}
    }

    return false;
}

int
gfx_getKey(bool block)
{
    int		key = 0;
    TCOD_key_t  tkey;

    do
    {
	TCODConsole::flush();
	tkey = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
	key = cookKey(tkey);
    } while (block && !key);

    return key;
}

void
gfx_update()
{
    TCODConsole::flush();
}

void
gfx_init()
{
    glbPulseNoise = new TCODNoise(1, 0.5, 2.0);
}

void
gfx_updatepulsetime()
{
    int 	timems = TCOD_sys_elapsed_milli();
    float	t;

    // Wrap every minute to keep noise in happy space.
    timems %= 60 * 1000;

    t = timems / 1000.0F;

    glbPulseVal = glbPulseNoise->getTurbulenceWavelet(&t, 6) * 2 + 1.0f;
}


void 
gfx_printchar(int x, int y, u8 c, ATTR_NAMES attr)
{
    gfx_printcodepoint(x, y, c, attr);
}

void
gfx_printcodepoint(int x, int y, unsigned int c, ATTR_NAMES attr)
{
    TCODColor		fg(glb_attrdefs[attr].fg_r,
			   glb_attrdefs[attr].fg_g,
			   glb_attrdefs[attr].fg_b);

    if (glb_attrdefs[attr].pulse)
	fg = fg * glbPulseVal;

    TCODConsole::root->setBack(x, y, TCODColor(glb_attrdefs[attr].bg_r,
					       glb_attrdefs[attr].bg_g,
					       glb_attrdefs[attr].bg_b));
    TCODConsole::root->setFore(x, y, fg);
    TCODConsole::root->setChar(x, y, c);
}

unsigned int
gfx_utf8next(const char **text)
{
    const unsigned char *src = (const unsigned char *) *text;
    unsigned int c;

    if (!src || !*src)
	return 0;

    if (src[0] < 0x80)
    {
	c = src[0];
	*text += 1;
    }
    else if ((src[0] & 0xe0) == 0xc0 && (src[1] & 0xc0) == 0x80)
    {
	c = ((src[0] & 0x1f) << 6) | (src[1] & 0x3f);
	*text += 2;
	if (c < 0x80)
	    c = 0xfffd;
    }
    else if ((src[0] & 0xf0) == 0xe0 &&
	     (src[1] & 0xc0) == 0x80 && (src[2] & 0xc0) == 0x80)
    {
	c = ((src[0] & 0x0f) << 12) | ((src[1] & 0x3f) << 6) |
	    (src[2] & 0x3f);
	*text += 3;
	if (c < 0x800 || (c >= 0xd800 && c <= 0xdfff))
	    c = 0xfffd;
    }
    else if ((src[0] & 0xf8) == 0xf0 &&
	     (src[1] & 0xc0) == 0x80 && (src[2] & 0xc0) == 0x80 &&
	     (src[3] & 0xc0) == 0x80)
    {
	c = ((src[0] & 0x07) << 18) | ((src[1] & 0x3f) << 12) |
	    ((src[2] & 0x3f) << 6) | (src[3] & 0x3f);
	*text += 4;
	if (c < 0x10000 || c > 0x10ffff)
	    c = 0xfffd;
    }
    else
    {
	c = 0xfffd;
	*text += 1;
    }

    return c;
}

int
gfx_utf8width(const char *text)
{
    int width = 0;

    while (text && *text)
    {
	gfx_utf8next(&text);
	width++;
    }

    return width;
}

void 
gfx_printattr(int x, int y, ATTR_NAMES attr)
{
    TCODConsole::root->setBack(x, y, TCODColor(glb_attrdefs[attr].bg_r,
					       glb_attrdefs[attr].bg_g,
					       glb_attrdefs[attr].bg_b));
    TCODConsole::root->setFore(x, y, TCODColor(glb_attrdefs[attr].fg_r,
					       glb_attrdefs[attr].fg_g,
					       glb_attrdefs[attr].fg_b));
}

void 
gfx_printchar(int x, int y, u8 c, u8 r, u8 g, u8 b)
{
    TCODConsole::root->setBack(x, y, TCODColor(0, 0, 0));
    TCODConsole::root->setFore(x, y, TCODColor(r, g, b));
    TCODConsole::root->setChar(x, y, c);
}


void 
gfx_printchar(int x, int y, u8 c, u8 r, u8 g, u8 b, u8 br, u8 bg, u8 bb)
{
    TCODConsole::root->setBack(x, y, TCODColor(br, bg, bb));
    TCODConsole::root->setFore(x, y, TCODColor(r, g, b));
    TCODConsole::root->setChar(x, y, c);
}

void 
gfx_getString(int x, int y, ATTR_NAMES attr, char *buf, int maxlen)
{
    strcpy(buf, "Not Implemented!");
}
