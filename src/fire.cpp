/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        fire.cpp ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *		Runs a fire simulation on another thread.
 *		Allows you to get pure info dumps of the state.
 */

#include <libtcod.hpp>
#include <math.h>
#include <assert.h>

#include "fire.h"

#include "rand.h"

FIRETEX::FIRETEX(int w, int h)
{
    myW = w;
    myH = h;
    myRGB = new u8[w * h * 3];
    memset(myRGB, 0, w * h * 3);
    myRefCnt.set(0);
}

FIRETEX::~FIRETEX()
{
    delete [] myRGB;
}

void
FIRETEX::decRef()
{
    if (myRefCnt.add(-1) <= 0)
	delete this;
}

void
FIRETEX::incRef()
{
    myRefCnt.add(1);
}

void
FIRETEX::redraw(int sx, int sy) const
{
    int		x, y;
    FORALL_XY(x, y)
    {
	TCODConsole::root->setBack(x+sx, y+sy, TCODColor(getR(x,y), getG(x,y), getB(x,y)));
    }
}

void
FIRETEX::firecurve(FireCurve firetype, float heat,
		    u8 *rgb)
{
    float	r, g, b;

    switch (firetype)
    {
	case FIRE_BLACKBODY:
	    r = heat * 255 * 3;
	    g = (heat - 1/3.0f) * 255 * 2;
	    b = (heat - 2/3.0f) * 255 * 1;
	    break;
	case FIRE_ICE:
	    b = heat * 255 * 3;
	    g = (heat - 1/3.0f) * 255 * 2;
	    r = (heat - 2/3.0f) * 255 * 1;
	    break;
	case FIRE_MONO:
	    r = g = b = heat * 255;
	    break;
    }

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    rgb[0] = (u8) r;
    rgb[1] = (u8) g;
    rgb[2] = (u8) b;
}

void
FIRETEX::buildFromConstant(float cutoff, FireCurve firetype)
{
    int		x, y;
    int		ycut = height() - height() * cutoff;
    float	heat;

    for (y = 0; y < height(); y++)
    {
	heat = y / (float)height();
	heat *= 1.5;
	if (y < ycut)
	    heat = 0;

	u8		rgb[3];

	firecurve(firetype, heat, rgb);

	for (x = 0; x < width(); x++)
	{
	    setRGB(x, y, rgb[0], rgb[1], rgb[2]);
	}
    }
}

void
FIRETEX::buildFromField(FIREFIELD *field, FireCurve firetype)
{
    int		x, y;
    float	fx, fy;
    float	cvtx, cvty;
    float	heat;

    cvtx = field->width() / (float) width();
    cvty = field->height() / (float) height();
    FORALL_XY(x, y)
    {
	fx = x * cvtx;
	fy = y * cvty;

	// Fire we like sharp so we just point sample.
	// That is my excuse for my laziness here :>

	heat = field->val(fx, fy);

	u8		rgb[3];

	firecurve(firetype, heat, rgb);

	setRGB(x, y, rgb[0], rgb[1], rgb[2]);
    }
}

FIREFIELD::FIREFIELD(int w, int h)
{
    myData = new float[w * h];
    myW = w;
    myH = h;
    constant(0);
}

FIREFIELD::~FIREFIELD()
{
    delete [] myData;
}

FIREFIELD::FIREFIELD(const FIREFIELD &field)
{
    myData = 0;
    *this = field;
}

FIREFIELD &
FIREFIELD::operator=(const FIREFIELD &field)
{
    delete [] myData;
    myW = field.myW;
    myH = field.myH;
    myData = new float[myW * myH];
    memcpy(myData, field.myData, myW*myH*sizeof(float));

    return *this;
}

void
FIREFIELD::constant(float v)
{
    if (v == 0)
    {
	memset(myData, 0, sizeof(float) * myW * myH);
	return;
    }

    int		x, y;

    FORALL_XY(x, y)
    {
	setVal(x, y, v);
    }
}

void
FIREFIELD::setVal(int ix, int iy, float v)
{
    assert(ix >= 0 && ix < myW);
    assert(iy >= 0 && iy < myH);
    myData[ix + iy * width()] = v;
}

float
FIREFIELD::getVal(int ix, int iy) const
{
    if (ix < 0)
	ix = 0;
    if (ix >= width())
	ix = width()-1;
    if (iy < 0)
	iy = 0;
    if (iy >= height())
	iy = height()-1;

    return myData[ix + iy * width()];
}

float
FIREFIELD::val(float x, float y) const
{
    int		ix = (int) floor(x);
    int		iy = (int) floor(y);

    float	fx = x - ix;
    float	fy = y - iy;
    float	v1, v2, a, b;

    a = getVal(ix, iy);
    b = getVal(ix+1, iy);
    v1 = LERP(a, b, fx);
    a = getVal(ix, iy+1);
    b = getVal(ix+1, iy+1);
    v2 = LERP(a, b, fx);

    return LERP(v1, v2, fy);
}

void
FIREFIELD::advect(FIREFIELD *src, FIREFIELD **v, float tinc)
{
    float		vx, vy;
    float		x, y, steplen, substep, finc;
    int			ix, iy;

    FORALL_XY(ix, iy)
    {
	x = (float) ix;
	y = (float) iy;

	vx = v[0]->getVal(ix, iy);
	vy = v[1]->getVal(ix, iy);
	finc = tinc;
	
	// The following is a cute way to ensure CFL conditions
	// are met while particle tracing.  Unfortunately, it means slow
	// fire can't ever catch up as the time to compute a frame is linear
	// in tinc, prohibiting us from just taking larger steps!
#if 0
	while (1)
	{
	    steplen = ABS(vx*finc) + ABS(vy*finc);
	    if (steplen < 1.0)
	    {
		// Run a single step.
		x -= vx*finc;
		y -= vy*finc;
		setVal(ix, iy, src->val(x, y));
		break;
	    }

	    // Find step size
	    substep = finc / steplen;
	    finc -= substep;
	    
	    x -= vx*substep;
	    y -= vy*substep;
	    vx = v[0]->val(x, y);
	    vy = v[1]->val(x, y);
	}
#else
	// A single step.
	x -= vx*finc;
	y -= vy*finc;
	setVal(ix, iy, src->val(x, y));
#endif
    }
}

static void *
fire_threadstarter(void *vdata)
{
    FIRE	*fire = (FIRE *) vdata;

    fire->mainLoop();

    return 0;
}


FIRE::FIRE(int w, int h, int tw, int th, FireCurve firetype)
{
    myW = w;
    myH = h;
    myResizePending = false;

    myDisableFire = false;
    if (w == 0 || h == 0)
    {
	myW = myH = 1;
	myDisableFire = true;
    }

    myTexW = tw;
    myTexH = th;
    myFireType = firetype;

    myDecay = new FIREFIELD(myW, myH);
    myHeat = new FIREFIELD(myW, myH);

    myV[0] = new FIREFIELD(myW, myH);
    myV[1] = new FIREFIELD(myW, myH);

    myTex = new FIRETEX(myTexW, myTexH);
    myTex->incRef();

    myFlameSize = 1.0;

    myNoise = new TCODNoise(3, 0.5, 2.0);
    myNoise3d = new TCODNoise(3);

    // Force the noise to init in case it does JIT as we must be on
    // main thread to use its RNG>
    float	f[3] = { 0, 0 };
    myNoise->getTurbulenceWavelet(f, 4);
    myNoise3d->getWavelet(f);

    myThread = THREAD::alloc();
    myThread->start(fire_threadstarter, this);
}

FIRE::~FIRE()
{
    delete myDecay;
    delete myHeat;
    delete myV[0];
    delete myV[1];

    myTex->decRef();
}

void
FIRE::updateTex(FIRETEX *tex)
{
    AUTOLOCK	a(myTexLock);

    tex->incRef();
    myTex->decRef();
    myTex = tex;
}

FIRETEX *
FIRE::getTex()
{
    AUTOLOCK	a(myTexLock);

    myTex->incRef();

    return myTex;
}

void
FIRE::setFlameSize(float size)
{
    // LEt us hope atomic :>
    myFlameSize = size;
}

void
FIRE::mainLoop()
{
    int		lastms, ms;
    float	tinc, t;
    float	flamesize;
    float	speedscale;
    float	decayrate;
    float	timetotop = 2.0;
    float	v, noise;
    float	np[3];
    
    int		x, y;
    int		seedstarty;
    FIREFIELD	*temp;

    temp = new FIREFIELD(width(), height());

    lastms = TCOD_sys_elapsed_milli();

    // Scale vertical speed so 2 sec = full height.
    speedscale = height() / timetotop;

    t = 0.0;

    seedstarty = myH - 10;

    while (1)
    {
	// Check if we have to rebuild everything.
	if (myResizePending)
	{
	    myResizePending = false;
	    delete myDecay;
	    delete myHeat;
	    delete myV[0];
	    delete myV[1];
	    delete temp;

	    myW = myResizeW;
	    myH = myResizeH;
	    myDisableFire = false;
	    if (myW == 0 || myH == 0)
	    {
		myW = 1;
		myH = 1;
		myDisableFire = true;
	    }

	    myDecay = new FIREFIELD(myW, myH);
	    myHeat = new FIREFIELD(myW, myH);

	    myV[0] = new FIREFIELD(myW, myH);
	    myV[1] = new FIREFIELD(myW, myH);
	    temp = new FIREFIELD(width(), height());
	
	    seedstarty = myH - 10;
	    speedscale = height() / timetotop;
	}

	ms = TCOD_sys_elapsed_milli();

	// Clamp this at 20fps.
	if (ms - lastms < 10)
	{
	    TCOD_sys_sleep_milli(1);
	    continue;
	}

	tinc = (ms - lastms) / 1000.0F;
	t += tinc;

	// Noise functions get crappy to far from 0.
	// This adds a "beat" of one minute.  Yeah!  That is why!
	if (t > 60) t -= 60;

	lastms = ms;

	// Read only once as unlocked..
	flamesize = myFlameSize;

	if (myDisableFire)
	{
	    // Rebuild our texture...
	    FIRETEX		*tex;
	    tex = new FIRETEX(myTexW, myTexH);
	    tex->buildFromConstant(flamesize, myFireType);

	    // Publish result.
	    updateTex(tex);
	}
	else
	{
	    // New decay rate is set so we hit 0 flameheight % of the way up.
	    // It will take timetoptop * flamesize to get to where we want to
	    // decay.  Clamp for flamesize < 0.01.
	    if (flamesize < 0.01)
		flamesize = 0.01F;
	    decayrate = 1 / (timetotop * flamesize);

	    // Source in our new decay & heat values.
	    FORALL_XY(x, y)
	    {
		if (y < seedstarty)
		    continue;

		myDecay->setVal(x, y, decayrate);
		v = myHeat->getVal(x, y);
		np[0] = (5.0F*x) / myW;
		np[1] = (5.0F*y) / myW;	// Yes, myW.
		np[2] = t;	

		noise = myNoise3d->getWavelet(np) * 2.0f + 0.75f;

		// Scale noise down at sides.
		float 	sidescale;
		// 1 at edges, 0 at center
		sidescale = ABS(width()/2.0f - x);
		sidescale /= (width() * 0.35f);
		// make 1 at center
		sidescale = 1.0f - sidescale;
		// bounce up
		sidescale *= 4.0F;
		// Clamp
		if (sidescale > 1)
		    sidescale = 1;
		if (sidescale < 0)
		    sidescale = 0;

		noise *= sidescale;

		// Now scale at bottom
		sidescale = 1.0F - (myH - y) / (float)(myH - seedstarty);
		sidescale *= 4.0F;
		if (sidescale > 1)
		    sidescale = 1;
		noise *= sidescale;

		v = MAX(v, noise);
		// v = noise;
		myHeat->setVal(x, y, v);
	    }

#if 1
	    // Age the heat field by the decay field.
	    FORALL_XY(x, y)
	    {
		v = myHeat->getVal(x, y);
		v -= myDecay->getVal(x, y) * tinc;
		if (v < 0)
		    v = 0;
		myHeat->setVal(x, y, v);
	    }

	    // Calculate velocity field.
	    myV[0]->constant(0);
	    myV[1]->constant(-speedscale);
	    FORALL_XY(x, y)
	    {
		np[0] = (1.0F*x) / myW + 30;
		np[1] = (1.0F*y) / myW;	// Yes, myW.
		np[1] += t * speedscale;	// Effective advection.
		np[2] = t;

		noise = myNoise->getTurbulenceWavelet(np, 4) - 0.5f;
		v = myV[0]->getVal(x, y);
		v += noise * speedscale * 1.7F;
		myV[0]->setVal(x, y, v);

		np[0] += 30;
		noise = myNoise->getTurbulenceWavelet(np, 4) - 0.5f;
		v = myV[1]->getVal(x, y);
		v += noise * speedscale * 1.7F;
		myV[1]->setVal(x, y, v);
	    }

	    // Advect our fields.
	    *temp = *myDecay;
	    myDecay->advect(temp, myV, tinc);
	    *temp = *myHeat;
	    myHeat->advect(temp, myV, tinc);

#endif

	    // Rebuild our texture...
	    FIRETEX		*tex;
	    tex = new FIRETEX(myTexW, myTexH);
	    tex->buildFromField(myHeat, myFireType);

	    // Publish result.
	    updateTex(tex);
	}
    }

    delete temp;
}
