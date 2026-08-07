/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        config.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#ifndef __config__
#define __config__

class CONFIG
{
public:
    CONFIG();
    ~CONFIG();

    void	load(const char *fname);
    int		flameWidth() const { return myFlameWidth; }
    int		flameHeight() const { return myFlameHeight; }

    bool	musicEnable() const { return myMusicEnable; }
    const char *musicFile() const { return myMusicFile; }
    int		musicVolume() const { return myMusicVolume; }

    bool	screenFull() const { return myFullScreen; }

private:
    int		myFlameWidth;
    int		myFlameHeight;
    int 	myMusicVolume;
    bool	myMusicEnable;
    const char *myMusicFile;

    bool		myFullScreen;
};

extern CONFIG *glbConfig;


#endif
