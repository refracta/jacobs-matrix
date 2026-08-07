/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        text.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "text.h"

#include <fstream>
using namespace std;

#include "ptrlist.h"
#include "grammar.h"
#include "rand.h"
#include "language.h"

PTRLIST<char *> glbTextEntry;
PTRLIST<char *> glbTextKey;
PTRLIST<char *> glbKoreanTextEntry;
PTRLIST<char *> glbKoreanTextKey;

char *
text_append(char *oldtxt, const char *append)
{
    int		len;
    char	*txt;
    
    len = (int)strlen(oldtxt) + (int)strlen(append) + 5;
    txt = (char *) malloc(len);
    strcpy(txt, oldtxt);
    strcat(txt, append);
    free(oldtxt);
    return txt;
}

void
text_striplf(char *line)
{
    while (*line)
    {
	if (*line == '\n' || *line == '\r')
	{
	    *line = '\0';
	    return;
	}
	line++;
    }
}

bool
text_hasnonws(const char *line)
{
    while (*line)
    {
	if (!ISSPACE(*line))
	    return true;
	line++;
    }
    return false;
}

int
text_firstnonws(const char *line)
{
    int		i = 0;
    while (line[i])
    {
	if (!ISSPACE(line[i]))
	    return i;
	i++;
    }
    return i;
}

int
text_lastnonws(const char *line)
{
    int		i = 0;
    while (line[i])
	i++;

    while (i > 0)
    {
	if (line[i] && !ISSPACE(line[i]))
	    return i;
	i--;
    }
    return i;
}

static void
text_load(const char *filename, PTRLIST<char *> &keys, PTRLIST<char *> &entries)
{
    ifstream	is(filename);
    char	line[500];
    bool	hasline = false;
    char	*text;

    while (hasline || is.getline(line, 500))
    {
	text_striplf(line);
	hasline = false;

	// Ignore comments.
	if (line[0] == '#')
	    continue;

	// See if an entry...
	if (!ISSPACE(line[0]))
	{
	    // This line is a key.
	    keys.append(strdup(line));
	    // Rest is the message...
	    text = strdup("");
	    while (is.getline(line, 500))
	    {
		text_striplf(line);
		int		firstnonws;

		for (firstnonws = 0; line[firstnonws] && ISSPACE(line[firstnonws]); firstnonws++);

		if (!line[firstnonws])
		{
		    // Completely blank line - insert a hard return!
		    text = text_append(text, "\n\n");
		}
		else if (!firstnonws)
		{
		    // New dictionary entry, break out!
		    hasline = true;
		    break;
		}
		else
		{
		    // Allow some ascii art...
		    if (firstnonws > 4)
		    {
			text = text_append(text, "\n");
			firstnonws = 4;
		    }

		    // Append remainder.
		    // Determine if last char was end of sentence.
		    if (*text && text[strlen(text)-1] != '\n')
		    {
			if (gram_isendsentence(text[strlen(text)-1]))
			    text = text_append(text, " ");
			text = text_append(text, " ");
		    }

		    text = text_append(text, &line[firstnonws]);
		}
	    }

	    // Append the resulting text.
	    entries.append(text);
	}
    }
}

void
text_init()
{
#ifdef __EMSCRIPTEN__
    text_load("/text.txt", glbTextKey, glbTextEntry);
    text_load("/text_ko.txt", glbKoreanTextKey, glbKoreanTextEntry);
#else
    text_load("../text.txt", glbTextKey, glbTextEntry);
    text_load("../text_ko.txt", glbKoreanTextKey, glbKoreanTextEntry);
#endif
}

void
text_shutdown()
{
    int		i;

    for (i = 0; i < glbTextKey.entries(); i++)
    {
	free(glbTextKey(i));
	free(glbTextEntry(i));
    }

    for (i = 0; i < glbKoreanTextKey.entries(); i++)
    {
	free(glbKoreanTextKey(i));
	free(glbKoreanTextEntry(i));
    }
}

static const char *
text_find(const char *key, PTRLIST<char *> &keys, PTRLIST<char *> &entries)
{
    int i;

    for (i = 0; i < keys.entries(); i++)
    {
	if (!strcmp(key, keys(i)))
	    return entries(i);
    }

    return 0;
}

BUF
text_lookup(const char *key)
{
    BUF		 buf;
    const char *entry = 0;

    if (language_is_korean())
	entry = text_find(key, glbKoreanTextKey, glbKoreanTextEntry);

    if (!entry)
	entry = text_find(key, glbTextKey, glbTextEntry);

    if (entry)
    {
	buf.reference(entry);
	return buf;
    }

    buf.sprintf("Missing text entry: \"%s\".", key);

    return buf;
}

BUF
text_lookup(BUF buf)
{
    return text_lookup(buf.buffer());
}

BUF
text_lookup(const char *dict, const char *word)
{
    BUF		buf;

    buf.sprintf("%s::%s", dict, word);
    return text_lookup(buf);
}

BUF
text_lookup(const char *dict, const char *word, const char *subword)
{
    BUF		buf;

    buf.sprintf("%s::%s::%s", dict, word, subword);
    return text_lookup(buf);
}
