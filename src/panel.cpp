/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        panel.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "panel.h"

#include "rand.h"
#include "gfxengine.h"
#include "language.h"

#include <ctype.h>
#include <stdlib.h>
#include <memory.h>

PANEL::PANEL(int w, int h, bool recordhistory)
{
    int		i;

    myX = 0;
    myY = 0;
    myW = w;    
    myH = h;
    myIndent = 0;
    myRightMargin = 1;
    myRecordHistory = recordhistory;

    myBorder = false;
    
    myLines = new unsigned int *[h];
    for (i = 0; i < h; i++)
    {
	myLines[i] = new unsigned int [w+1];
	myLines[i][0] = '\0';
    }

    myAttrMap = new ATTR_NAMES *[h];
    for (i = 0; i < h; i++)
    {
	myAttrMap[i] = new ATTR_NAMES [w];
    }

    myCurLine = 0;
    myCurPos = 0;

    setAttr(ATTR_NORMAL);
}

PANEL::~PANEL()
{
    int		i;

    for (i = 0; i < myH; i++)
	delete [] myLines[i];
    delete [] myLines;
    for (i = 0; i < myH; i++)
	delete [] myAttrMap[i];
    delete [] myAttrMap;

    clearHistory();
}

void
PANEL::setBorder(bool enable, u8 sym, ATTR_NAMES attr)
{
    myBorder = enable;
    myBorderSym = sym;
    myBorderAttr = attr;
}

void
PANEL::clearHistory()
{
    AUTOLOCK	a(myLock);
    int		i;

    for (i = 0; i < myHistory.entries(); i++)
	delete [] myHistory(i);
    myHistory.clear();
}

int
PANEL::getHistoryLine()
{
    AUTOLOCK	a(myLock);
    return myCurLine + myHistory.entries();
}

void
PANEL::scrollToHistoryLine(int line)
{
    AUTOLOCK	a(myLock);
    // Determine how many lines of history we need to scroll into
    // our window...
    int		netnew;

    netnew = getHistoryLine() - line;
    if (netnew <= 0)
    {
	// Either trying to go forward in time or a no-op
	return;
    }

    while (netnew--)
    {
	scrollDown();
    }
}

void
PANEL::setCurrentLine(const char *text)
{
    AUTOLOCK	a(myLock);
    // Clear out the current line.
    myLines[myCurLine][0] = '\0';
    myCurPos = 0;

    // And append normally
    appendText(text);
}

void
PANEL::appendText(const char *text, int linecount)
{
    if (!text)
	return;

    AUTOLOCK	a(myLock);
    while (*text)
    {
	const char *word = text;
	const char *scan;
	unsigned int codepoint = gfx_utf8next(&text);

	if (codepoint == '\r')
	    continue;

	if (codepoint == '\n')
	{
	    myLines[myCurLine][myCurPos] = 0;
	    wrappedNewLine(linecount);
	    continue;
	}

	if (codepoint == ' ' || codepoint == '\t')
	{
	    if (myCurPos < myW - myRightMargin)
	    {
		myLines[myCurLine][myCurPos] = ' ';
		myAttrMap[myCurLine][myCurPos] = myTextAttr;
		myCurPos++;
		myLines[myCurLine][myCurPos] = 0;
	    }
	    continue;
	}

	// Determine the length of this word in console cells.  UTF-8 bytes do
	// not correspond to visible character positions.
	int wordlen = 0;
	scan = word;
	while (*scan)
	{
	    const char *before = scan;
	    codepoint = gfx_utf8next(&scan);
	    if (codepoint == ' ' || codepoint == '\t' ||
		codepoint == '\r' || codepoint == '\n')
	    {
		scan = before;
		break;
	    }
	    wordlen++;
	}

	if (myCurPos > myIndent &&
	    myCurPos + wordlen > myW - myRightMargin)
	    wrappedNewLine(linecount);

	text = word;
	while (*text)
	{
	    const char *before = text;
	    codepoint = gfx_utf8next(&text);
	    if (codepoint == ' ' || codepoint == '\t' ||
		codepoint == '\r' || codepoint == '\n')
	    {
		text = before;
		break;
	    }

	    if (myCurPos >= myW - myRightMargin)
		wrappedNewLine(linecount);

	    myLines[myCurLine][myCurPos] = codepoint;
	    myAttrMap[myCurLine][myCurPos] = myTextAttr;
	    myCurPos++;
	    myLines[myCurLine][myCurPos] = 0;
	}
    }
}

void
PANEL::wrappedNewLine(int &linecount)
{
    linecount++;
    newLine();
    if (myH <= 2)
    {
	// Cannot scroll with prompt!
	linecount = 0;
    }
    else if (linecount >= myH-2)
    {
	awaitKey();
	linecount = 0;
    }
}

void redrawWorld();

void
PANEL::awaitKey()
{
    int moreline = myCurLine;
    int morepos = myCurPos;

    // Draw the pagination prompt on the otherwise empty current line.  Do
    // not add a newline: after the keypress we erase this temporary text so
    // it cannot remain embedded in the following page.
    appendText(language_is_korean() ? "-- 계속 --" : "-- MORE --");
    while (!gfx_getKey(false))
    {
	redrawWorld();
    }

    myLines[moreline][morepos] = 0;
    myCurLine = moreline;
    myCurPos = morepos;
}

void
PANEL::getString(const char *prompt, char *buf, int maxlen)
{
    appendText(prompt);

    redraw();

    // Clamp our string to our panel width as we don't handle scrolling.
    if (maxlen + myCurPos > myW)
    {
	maxlen = myW - myCurPos;
    }
    
    gfx_getString(myCurPos + myX, myCurLine + myY, myAttrMap[myCurLine][myCurPos], buf, maxlen);
    // Ensure the buffer is inside our panel's memory!
    appendText(buf);
    // Always do a new line!
    newLine();
}

void
PANEL::setAttr(ATTR_NAMES attr)
{
    fillRect(0, 0, myW, myH, attr);
    myTextAttr = attr;
}

void
PANEL::fillRect(int x, int y, int w, int h, ATTR_NAMES attr)
{
    AUTOLOCK	a(myLock);
    int		i, j;

    // Clip...
    if (x < 0)
    {
	w += x;
	x = 0;
    }
    if (y < 0)
    {
	h += y;
	y = 0;
    }

    for (i = y; i < y + h; i++)
    {
	if (i >= myH)
	    break;
	for (j = x; j < x + w; j++)
	{
	    if (j >= myW)
		break;

	    myAttrMap[i][j] = attr;
	}
    }
}

void
PANEL::newLine()
{
    AUTOLOCK	a(myLock);
    myCurLine++;
    if (myCurLine == myH)
    {
	scrollUp();
    }
    myCurPos = 0;
    int		i;
    for (i = 0; i < myIndent; i++)
	appendText(" ");
}

void
PANEL::clear()
{
    AUTOLOCK	a(myLock);
    int		i;

    for (i = 0; i < myH; i++)
    {
	myLines[i][0] = '\0';
    }
    myCurPos = 0;
    myCurLine = 0;

    // Important we start with our indent!
    for (i = 0; i < myIndent; i++)
	appendText(" ");

    setAttr(myTextAttr);
}

void
PANEL::move(int x, int y)
{
    myX = x;
    myY = y;
}

void
PANEL::redraw()
{
    AUTOLOCK	a(myLock);
    int		x, y;

    for (y = 0; y < myH; y++)
    {
	if (y + myY >= 0 && y + myY < SCR_HEIGHT)
	{
	    for (x = 0; x < myW; x++)
	    {
		// Hit end of line.
		if (!myLines[y][x])
		    break;

		if (x + myX < 0 || x + myX >= SCR_WIDTH)
		    continue;
		gfx_printcodepoint(x + myX, y + myY, myLines[y][x], myAttrMap[y][x]);
	    }

	    // Pad with spaces.
	    for (; x < myW; x++)
	    {
		if (x + myX < 0 || x + myX >= SCR_WIDTH)
		    continue;
		gfx_printchar(x + myX, y + myY, ' ', myAttrMap[y][x]);
	    }
	}
    }

    if (myBorder)
    {
	for (y = -1; y < myH+1; y++)
	{
	    if (y + myY < 0 || y + myY >= SCR_HEIGHT)
		continue;

	    if (myX-1 >= 0 && myX-1 < SCR_WIDTH)
		gfx_printchar(myX-1, y + myY, myBorderSym, myBorderAttr);
	    if (myX+myW >= 0 && myX+myW < SCR_WIDTH)
		gfx_printchar(myX+myW, y + myY, myBorderSym, myBorderAttr);
	}
	for (x = -1; x < myW+1; x++)
	{
	    if (x + myX < 0 || x + myX >= SCR_WIDTH)
		continue;

	    if (myY-1 >= 0 && myY-1 < SCR_HEIGHT)
		gfx_printchar(x + myX, myY - 1, myBorderSym, myBorderAttr);
	    if (myY+myH >= 0 && myY+myH < SCR_HEIGHT)
		gfx_printchar(x + myX, myY+myH, myBorderSym, myBorderAttr);
	}
    }
}

void
PANEL::setIndent(int indent)
{
    myIndent = indent;
}

void
PANEL::setRigthMargin(int margin)
{
    myRightMargin = margin;
}

void
PANEL::scrollUp()
{
    AUTOLOCK	a(myLock);
    int		x, y;

    if (myRecordHistory)
    {
	unsigned int	*line;

	// It is a sign of an incompetent programmer to have an oversized
	// overflow like this.  Either you know the behaviour, so can
	// have an exact bonus, or you don't, so can't really give any
	// bonus.
	line = new unsigned int [myW+1];
	memcpy(line, myLines[0], (myW+1) * sizeof(unsigned int));
	myHistory.append(line);
    }

    for (y = 1; y < myH; y++)
    {
	memcpy(myLines[y-1], myLines[y], (myW+1) * sizeof(unsigned int));
	memcpy(myAttrMap[y-1], myAttrMap[y], myW * sizeof(ATTR_NAMES));
    }
    myLines[myH-1][0] = '\0';
    for (x = 0; x < myW; x++)
    {
	myAttrMap[myH-1][x] = myTextAttr;
    }


    myCurLine--;
    if (myCurLine < 0)
	myCurLine = 0;
}

void
PANEL::scrollDown()
{
    AUTOLOCK	a(myLock);
    int		x, y;

    for (y = myH-1; y >= 1; y--)
    {
	memcpy(myLines[y], myLines[y-1], (myW+1) * sizeof(unsigned int));
	memcpy(myAttrMap[y], myAttrMap[y-1], myW * sizeof(ATTR_NAMES));
    }
    for (x = 0; x < myW; x++)
    {
	myAttrMap[0][x] = myTextAttr;
    }

    // Pull out the history...
    if (myHistory.entries())
    {
	memcpy(myLines[0], myHistory.top(), (myW+1) * sizeof(unsigned int));
	delete [] myHistory.pop();
    }
    else
	myLines[y][0] = '\0';

    myCurLine++;
    if (myCurLine > myH-1)
	myCurLine = myH-1;
}
