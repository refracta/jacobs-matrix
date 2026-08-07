/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        config.cpp ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#include <libtcod.hpp>

#include "config.h"

CONFIG *glbConfig = 0;

CONFIG::CONFIG()
{
}

CONFIG::~CONFIG()
{
}

void
CONFIG::load(const char *fname)
{
    TCODParser		parser;
    TCODParserStruct	*flame, *music, *screen;

    flame = parser.newStructure("flame");
    flame->addProperty("width", TCOD_TYPE_INT, true);
    flame->addProperty("height", TCOD_TYPE_INT, true);

    music = parser.newStructure("music");
    music->addProperty("enable", TCOD_TYPE_BOOL, true);
    music->addProperty("file", TCOD_TYPE_STRING, true);
    music->addProperty("volume", TCOD_TYPE_INT, true);

    screen = parser.newStructure("screen");
    screen->addProperty("full", TCOD_TYPE_BOOL, true);

    parser.run(fname, 0);

    myFlameWidth = parser.getIntProperty("flame.width");
    myFlameHeight = parser.getIntProperty("flame.height");

    myMusicEnable = parser.getBoolProperty("music.enable");
    myMusicFile = parser.getStringProperty("music.file");
    myMusicVolume = parser.getIntProperty("music.volume");

    myFullScreen = parser.getBoolProperty("screen.full");
}
