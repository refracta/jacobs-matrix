/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        msg.cpp ( Live once Library, C++ )
 *
 * COMMENTS:
 */

#include "msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "panel.h"
#include "grammar.h"
#include "mob.h"
#include "item.h"
#include "buf.h"
#include "language.h"

#include "thread.h"

PANEL *glbPanel = 0;
LOCK	glbMsgLock;

bool glbBlankNewTurn = false;

void
msg_update()
{
    glbPanel->redraw();
}

void 
msg_registerpanel(PANEL *panel)
{
    glbPanel = panel;
}

int
msg_gethistoryline()
{
    return glbPanel->getHistoryLine();
}

void
msg_clearhistory()
{
    glbPanel->clearHistory();
}

void
msg_scrolltohistory(int line)
{
    AUTOLOCK	a(glbMsgLock);
    glbPanel->scrollToHistoryLine(line);

    // This is a hack for save scummer.  We know our last save point
    // was a > prompt, so we need to restore said prompt.
    glbPanel->setCurrentLine("> ");
    glbBlankNewTurn = true;
}

void
msg_report(const char *msg)
{
    AUTOLOCK	a(glbMsgLock);
    BUF		buf;
    buf.reference(language_text(msg));

    // Don't want to have to worry about appending all the time!
    if (gram_isendsentence(buf.lastchar()))
	buf.strcat("  ");
    
    glbPanel->appendText(buf);
    glbBlankNewTurn = false;
}

void
msg_getString(const char *prompt, char *buf, int len)
{
    glbPanel->getString(prompt, buf, len);
}

void
msg_quote(const char *msg)
{
    AUTOLOCK	a(glbMsgLock);
    glbPanel->setIndent(1);
    glbPanel->newLine();
    msg_report(msg);
    glbPanel->setIndent(0);
}

void
msg_newturn()
{
    AUTOLOCK	a(glbMsgLock);
    if (!glbBlankNewTurn)
    {
	glbBlankNewTurn = true;
	// We do not want double new lines!
	if (!glbPanel->atNewLine())
	    glbPanel->newLine();

	glbPanel->appendText("> ");
    }
}

//
// Builder functions that respect the triple possibilities.
//
VERB_PERSON
msg_getPerson(MOB *m, ITEM *i, const char *s)
{
    VERB_PERSON		person = VERB_IT;

    if (m)
    {
	person = m->getPerson();
    }
    else if (i)
    {
	person = i->getPerson();
    }
    else if (s)
    {
	person = VERB_IT;
	if (gram_isnameplural(s))
	    person = VERB_THEY;
    }

    return person;
}


BUF
msg_buildVerb(const char *verb, MOB *m_subject, ITEM *i_subject, const char *s_subject)
{
    VERB_PERSON		person;

    person = msg_getPerson(m_subject, i_subject, s_subject);

    return gram_conjugate(verb, person);
}

BUF
msg_buildReflexive(MOB *m, ITEM *i, const char *s)
{
    VERB_PERSON		p;
    BUF			buf;

    p = msg_getPerson(m, i, s);

    buf.reference(gram_getreflexive(p));
    return buf;
}

BUF
msg_buildPossessive(MOB *m, ITEM *i, const char *s)
{
    VERB_PERSON		p;
    BUF			buf;

    p = msg_getPerson(m, i, s);

    buf.reference(gram_getpossessive(p));
    return buf;
}

BUF
msg_buildFullName(MOB *m, ITEM *i, const char *s, bool usearticle = true)
{
    BUF			 rawname, buf;
    BUF			 result;

    rawname.reference("no tea");
    if (m)
    {
	rawname.reference(m->getName());
    }
    else if (i)
    {
	rawname = i->getName();
    }
    else if (s)
	rawname.reference(s);
    
    const char		*art;

    if (usearticle)
	art = gram_getarticle(rawname);
    else
	art = "";

    buf.sprintf("%s%s", art, rawname.buffer());

    return buf;
}

static BUF
msg_buildKoreanName(MOB *m, ITEM *i, const char *s)
{
    BUF rawname, result;

    rawname.reference("아무것도");
    if (m)
	rawname.reference(m->getName());
    else if (i)
	rawname = i->getName();
    else if (s)
	rawname.reference(s);

    // Copy the result: language_text() may return rawname's temporary buffer.
    result.strcpy(language_text(rawname.buffer()));
    return result;
}

static bool
msg_koreanHasBatchim(const char *text)
{
    const unsigned char *src = (const unsigned char *) text;
    unsigned int last = 0;

    while (src && *src)
    {
	if (*src < 0x80)
	{
	    last = *src++;
	}
	else if ((*src & 0xe0) == 0xc0 && src[1])
	{
	    last = ((src[0] & 0x1f) << 6) | (src[1] & 0x3f);
	    src += 2;
	}
	else if ((*src & 0xf0) == 0xe0 && src[1] && src[2])
	{
	    last = ((src[0] & 0x0f) << 12) | ((src[1] & 0x3f) << 6) |
		   (src[2] & 0x3f);
	    src += 3;
	}
	else if ((*src & 0xf8) == 0xf0 && src[1] && src[2] && src[3])
	{
	    last = ((src[0] & 0x07) << 18) | ((src[1] & 0x3f) << 12) |
		   ((src[2] & 0x3f) << 6) | (src[3] & 0x3f);
	    src += 4;
	}
	else
	{
	    src++;
	}
    }

    return last >= 0xac00 && last <= 0xd7a3 && ((last - 0xac00) % 28) != 0;
}

static void
msg_appendKoreanParticle(BUF &buf, BUF word, const char *batchim,
			 const char *no_batchim)
{
    buf.strcat(word);
    buf.strcat(msg_koreanHasBatchim(word.buffer()) ? batchim : no_batchim);
}

static bool
msg_formatKorean(BUF &buf, const char *msg,
		 MOB *m_subject, ITEM *i_subject, const char *s_subject,
		 const char *verb,
		 MOB *m_object, ITEM *i_object, const char *s_object)
{
    BUF subject = msg_buildKoreanName(m_subject, i_subject, s_subject);
    BUF object = msg_buildKoreanName(m_object, i_object, s_object);

    if (!strcmp(msg, "%S %v %O."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat(language_text(verb));
	buf.strcat(".  ");
    }
    else if (!strcmp(msg, "%S <miss> %O."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat("빗맞혔다.  ");
    }
    else if (!strcmp(msg, "%S <kill> %O!"))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat("죽였다!  ");
    }
    else if (!strcmp(msg, "%S <be> killed!"))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	buf.strcat("죽었다!  ");
    }
    else if (!strcmp(msg, "You see %O."))
    {
	msg_appendKoreanParticle(buf, object, "이 ", "가 ");
	buf.strcat("보인다.  ");
    }
    else if (!strcmp(msg, "You start acting like %O.  "))
    {
	buf.strcat("당신은 ");
	buf.strcat(object);
	buf.strcat("처럼 행동하기 시작했다.  ");
    }
    else if (!strcmp(msg, "%S <swing> at empty air!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("허공을 휘둘렀다!  ");
    }
    else if (!strcmp(msg, "%S <be> digested!"))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	buf.strcat("소화되고 있다!  ");
    }
    else if (!strcmp(msg, "%S <leap>."))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	buf.strcat("도약했다.  ");
    }
    else if (!strcmp(msg, "%S cannot find a path there."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("그곳으로 가는 길을 찾지 못했다.  ");
    }
    else if (!strcmp(msg, "%S <drop> %O."))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat("버렸다.  ");
    }
    else if (!strcmp(msg, "%S <drop> nothing."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("버릴 것이 없다.  ");
    }
    else if (!strcmp(msg, "%S <climb> down... to nowhere!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("아래로 내려갔지만... 아무 데도 갈 수 없었다!  ");
    }
    else if (!strcmp(msg, "%S <climb> up... to nowhere!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("위로 올라갔지만... 아무 데도 갈 수 없었다!  ");
    }
    else if (!strcmp(msg, "%S <climb> a tree... and <fall> down!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("나무를 올랐다가... 떨어졌다!  ");
    }
    else if (!strcmp(msg, "%S <see> nothing to climb here."))
    {
	buf.strcat("여기에는 올라갈 것이 없다.  ");
    }
    else if (!strcmp(msg, "%S <talk> to %O!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat(object); buf.strcat("에게 말을 걸었다!  ");
    }
    else if (!strcmp(msg, "%S <chat> with %O:\n"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	msg_appendKoreanParticle(buf, object, "과 대화했다:\n", "와 대화했다:\n");
    }
    else if (!strcmp(msg, "%S <steal> %O."))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat("훔쳤다.  ");
    }
    else if (!strcmp(msg, "%S <know> no spells!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("아는 주문이 없다!  ");
    }
    else if (!strcmp(msg, "%S <decide> not to aim at %O."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("자신을 겨누지 않기로 했다.  ");
    }
    else if (!strcmp(msg, "%S do not have enough room inside here."))
    {
	buf.strcat("이곳은 내부 공간이 부족하다.  ");
    }
    else if (!strcmp(msg, "%S <lack> sufficent mana."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("마나가 부족하다.  ");
    }
    else if (!strcmp(msg, "%S <fire> at empty air!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("허공에 발사했다!  ");
    }
    else if (!strcmp(msg, "The portal dissipates at %O."))
    {
	buf.strcat(object); buf.strcat("에 있던 차원문이 사라졌다.  ");
    }
    else if (!strcmp(msg, "%S <bump> into %O."))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	msg_appendKoreanParticle(buf, object, "과 부딪혔다.  ", "와 부딪혔다.  ");
    }
    else if (!strcmp(msg, "%S <reproduce>!"))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	buf.strcat("번식했다!  ");
    }
    else if (!strcmp(msg, "%S <be> blocked by %O."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	msg_appendKoreanParticle(buf, object, "에 ", "에 ");
	buf.strcat("가로막혔다.  ");
    }
    else if (!strcmp(msg, "%S <grope> the ground foolishly."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("어리석게 땅을 더듬었다.  ");
    }
    else if (!strcmp(msg, "%S <pick> up %O."))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat("주웠다.  ");
    }
    else if (!strcmp(msg, "%S <discard> it as junk."))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("쓸모없는 물건이라며 버렸다.  ");
    }
    else if (!strcmp(msg, "%S <be> blinded!"))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	buf.strcat("눈이 멀었다!  ");
    }
    else if (!strcmp(msg, "%S can see!"))
    {
	msg_appendKoreanParticle(buf, subject, "은 ", "는 ");
	buf.strcat("다시 볼 수 있다!  ");
    }
    else if (!strcmp(msg, "%S drink %O."))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat("마셨다.  ");
    }
    else if (!strcmp(msg, "%S replace %r %o."))
    {
	msg_appendKoreanParticle(buf, subject, "이 ", "가 ");
	msg_appendKoreanParticle(buf, object, "으로 ", "로 ");
	buf.strcat("장비를 교체했다.  ");
    }
    else if (!strcmp(msg, "You wear the boots."))
    {
	buf.strcat("당신은 장화를 신었다.  ");
    }
    else if (!strcmp(msg, "Your %O run out of mana.") ||
	     !strcmp(msg, "Your %o runs out of mana."))
    {
	msg_appendKoreanParticle(buf, object, "은 ", "는 ");
	buf.strcat("마나를 모두 소진했다.  ");
    }
    else if (!strcmp(msg, "You drink %O.  Your feet dance quickly!"))
    {
	buf.strcat("당신은 ");
	msg_appendKoreanParticle(buf, object, "을 ", "를 ");
	buf.strcat("마셨다. 발이 빠르게 움직인다!  ");
    }
    else if (!strcmp(msg, "Your movements slow."))
    {
	buf.strcat("움직임이 느려졌다.  ");
    }
    else if (!strcmp(msg, "Your muscles slow."))
    {
	buf.strcat("근육의 움직임이 둔해졌다.  ");
    }
    else if (!strcmp(msg, "You regain normal movement."))
    {
	buf.strcat("다시 정상적으로 움직일 수 있다.  ");
    }
    else if (!strcmp(msg, "You wear the ring."))
    {
	buf.strcat("당신은 반지를 꼈다.  ");
    }
    else if (!strcmp(msg, "You wear the cloak."))
    {
	buf.strcat("당신은 망토를 둘렀다.  ");
    }
    else
    {
	return false;
    }

    return true;
}

//
// This is the universal formatter
//
void
msg_format(const char *msg, MOB *m_subject, ITEM *i_subject, const char *s_subject, const char *verb, MOB *m_object, ITEM *i_object, const char *s_object)
{
    AUTOLOCK	a(glbMsgLock);
    BUF			 buf;
    BUF			 newtext;

    if (language_is_korean() &&
	msg_formatKorean(buf, msg, m_subject, i_subject, s_subject, verb,
			m_object, i_object, s_object))
    {
	msg_report(buf);
	return;
    }

    while (*msg)
    {
	if (*msg == '%')
	{
	    newtext.reference("");
	    switch (msg[1])
	    {
		case '%':
		    // Pure %.
		    newtext.reference("%");
		    break;

		case '<':
		    // Escapped <
		    newtext.reference("<");
		    break;
		    
		case 'v':
		    // Conjugate the given verb & append.
		    assert(verb);
		    newtext = msg_buildVerb(verb, m_subject, i_subject, s_subject);
		    break;

		case 'S':
		    newtext = msg_buildFullName(m_subject, i_subject, s_subject);
		    break;

		case 'r':
		    newtext = msg_buildPossessive(m_subject, i_subject, s_subject);
		    break;

		case 'O':
		    if (m_subject && (m_subject == m_object) ||
			i_subject && (i_subject == i_object))
		    {
			// Reflexive case!
			newtext = msg_buildReflexive(m_object, i_object, s_object);
		    }
		    else
			newtext = msg_buildFullName(m_object, i_object, s_object);
		    break;
		case 'o':
		    if (m_subject && (m_subject == m_object) ||
			i_subject && (i_subject == i_object))
		    {
			// Reflexive case!
			newtext = msg_buildReflexive(m_object, i_object, s_object);
		    }
		    else
			newtext = msg_buildFullName(m_object, i_object, s_object, false);
		    break;
	    }

	    msg += 2;
	    // Append the new text
	    buf.strcat(newtext);
	}
	else if (*msg == '<')
	{
	    char *v = strdup(&msg[1]);
	    char *startv = v;
	    
	    msg++;
	    while (*v && *v != '>')
	    {
		msg++;
		v++;
	    }
	    *v = 0;
	    // Must be closed!
	    assert(*msg == '>');
	    if (*msg == '>')
		msg++;

	    newtext = msg_buildVerb(startv, m_subject, i_subject, s_subject);

	    buf.strcat(newtext);

	    free(startv);
	}
	else
	{
	    // Normal character!
	    buf.append(*msg++);
	}
    }

    // If it ends with puntuation, add spaces.
    if (buf.isstring() && gram_isendsentence(buf.lastchar()))
    {
	buf.strcat("  ");
    }

    // Formatted into buf.  Capitalize & print.
    newtext = gram_capitalize(buf);

    msg_report(newtext);
}

//
// These are the specific instantiations.
//
void
msg_format(const char *msg, MOB *subject)
{
    msg_format(msg, subject, 0, 0, 0, 0, 0, 0);
}

void
msg_format(const char *msg, ITEM *subject)
{
    msg_format(msg, 0, subject, 0, 0, 0, 0, 0);
}

void
msg_format(const char *msg, MOB *subject, MOB *object)
{
    msg_format(msg, subject, 0, 0, 0, object, 0, 0);
}

void
msg_format(const char *msg, MOB *subject, ITEM *object)
{
    msg_format(msg, subject, 0, 0, 0, 0, object, 0);
}

void
msg_format(const char *msg, MOB *subject, const char *verb, MOB *object)
{
    msg_format(msg, subject, 0, 0, verb, object, 0, 0);
}

void
msg_format(const char *msg, MOB *subject, const char *object)
{
    msg_format(msg, subject, 0, 0, 0, 0, 0, object);
}
