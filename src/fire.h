/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        fire.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *		Runs a fire simulation on another thread.
 *		Allows you to get pure info dumps of the state.
 */

#ifndef __fire__
#define __fire__

#include <libtcod.hpp>

#include "thread.h"
#include "mygba.h"

class THREAD;
class FIREFIELD;

enum FireCurve
{
    FIRE_BLACKBODY,
    FIRE_ICE,
    FIRE_MONO,
};

class FIRETEX
{
public:
    FIRETEX(int w, int h);

    void		incRef();
    void		decRef();

    u8			getR(int x, int y) const
			{ return myRGB[x*3+y*myW*3]; }
    u8			getG(int x, int y) const
			{ return myRGB[x*3+y*myW*3+1]; }
    u8			getB(int x, int y) const
			{ return myRGB[x*3+y*myW*3+2]; }

    void		setRGB(int x, int y, u8 r, u8 g, u8 b)
			{
			    myRGB[x*3+y*myW*3] = r;
			    myRGB[x*3+y*myW*3+1] = g;
			    myRGB[x*3+y*myW*3+2] = b;
			}

    static void		firecurve(FireCurve firetype, float heat,
				u8 *rgb);

    void		buildFromConstant(float percent, FireCurve firetype);
    void		buildFromField(FIREFIELD *field, FireCurve firetype);

    int			width() const { return myW; }
    int			height() const { return myH; }

    void		redraw(int sx, int sy) const;

private:
    ~FIRETEX();

    ATOMIC_INT32		myRefCnt;

    u8			*myRGB;
    int			 myW, myH;
};

class FIREFIELD
{
public:
     FIREFIELD(int w, int h);
    ~FIREFIELD();

    FIREFIELD(const FIREFIELD &field);
    FIREFIELD &operator=(const FIREFIELD &field);

    void		constant(float v);
    void		setVal(int ix, int iy, float v);

    float		getVal(int ix, int iy) const;
    float		val(float x, float y) const;

    int			width() const { return myW; }
    int			height() const { return myH; }

    // Replaces this with src advected by v for tinc
    void		advect(FIREFIELD *src, FIREFIELD **v, float tinc);

private:
    float		*myData;
    int			 myW, myH;
};

class FIRE
{
public:
    FIRE(int w, int h, int tw, int th, FireCurve firetype);
    ~FIRE();

    void		 updateTex(FIRETEX *tex);
    // Returns a FIRETEX.  You must invoke decRef() on it.
    FIRETEX		*getTex();

    int			 width() const { return myW; }
    int			 height() const { return myH; }

    // Yes, this is poorly locked, hopefully it doesn't rearrange
    // our assignments on us.
    void		 resize(int nw, int nh)
			 { myResizeW = nw; myResizeH = nh; myResizePending = true; }

    void		 setFlameSize(float size);

    // Public only for call back conveninece.
    void		 mainLoop();

    void		 setFireType(FireCurve firetype)
			 { myFireType = firetype; }

private:
    THREAD		*myThread;

    // Heat decay per second
    FIREFIELD		*myDecay;
    // 0..1 heat val per voxel.
    FIREFIELD		*myHeat;
    // Velocity field.
    FIREFIELD		*myV[2];

    TCODNoise		*myNoise;
    TCODNoise		*myNoise3d;

    FIRETEX		*myTex;
    LOCK		 myTexLock;

    FireCurve		 myFireType;

    float		 myFlameSize;

    // Simulation res
    int			 myW, myH;
    // Disaply res
    int			 myTexW, myTexH;

    // Turns into boring bar graphs.
    bool		 myDisableFire;

    bool		 myResizePending;
    int			 myResizeW, myResizeH;
};

#endif
