/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        panel.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 *	Defines a text panel which can take scrolling text and handle
 *	all those icky line wrap issues.
 */

#ifndef __panel__
#define __panel__

#include "glbdef.h"
#include "ptrlist.h"
#include "buf.h"
#include "thread.h"

class PANEL
{
public:
	     PANEL(int w, int h, bool recordhistory = false);
    virtual ~PANEL();

    void	 appendText(const char *text, int linecount = 0);
    void	 appendText(BUF buf, int linecount = 0)
		    { appendText(buf.buffer(), linecount); }
    void	 newLine();

    // All net-new text will be so coloured.
    // It is supposed to also scroll.
    void	 setTextAttr(ATTR_NAMES attr) { myTextAttr = attr; }

    // Rewrites the current line accordingly.
    void	 setCurrentLine(const char *text);
    void	 setCurrentLine(BUF buf)
		    { setCurrentLine(buf.buffer()); }

    void	 clearHistory();

    int	 	 getHistoryLine();
    void	 scrollToHistoryLine(int line);

    // Prompts for a key to be hit & waits.
    void	 awaitKey();

    void	 setBorder(bool enable, u8 sym = ' ', ATTR_NAMES attr = ATTR_NONE);

    // Prints the prompt and then asks the user to input a string using
    // the remaining space.
    void	 getString(const char *prompt, char *buf, int maxlen);

    bool	 atNewLine() const
		 { return myCurPos == 0; }

    void	 move(int x, int y);

    void	 clear();

    void	 redraw();

    int		 getX() const { return myX; }
    int		 getY() const { return myY; }
    int		 getW() const { return myW; }
    int		 getH() const { return myH; }

    void	 setAttr(ATTR_NAMES attr);
    
    // Done in our coordinate space.
    // Updates only the attributes therein.
    void	 fillRect(int x, int y, int w, int h, ATTR_NAMES attr);

    void	 setIndent(int indent);
    void	 setRigthMargin(int margin);
    
protected:
    void	 scrollUp();
    void	 scrollDown();

    char	**myLines;
    int		 myX, myY, myW, myH;
    int		 myCurLine, myCurPos;
    int		 myIndent, myRightMargin;
    ATTR_NAMES	**myAttrMap;

    LOCK	 myLock;

    bool	 myBorder;
    ATTR_NAMES	 myBorderAttr;
    u8		 myBorderSym;

    ATTR_NAMES	 myTextAttr;

    bool		myRecordHistory;
    PTRLIST<char *>	myHistory;
};

#endif

