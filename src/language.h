/*
 * Runtime language selection for the web port.
 */

#ifndef __language__
#define __language__

enum LANGUAGE_NAMES
{
    LANGUAGE_ENGLISH,
    LANGUAGE_KOREAN,
    NUM_LANGUAGES
};

void language_init();
void language_set(LANGUAGE_NAMES language);
LANGUAGE_NAMES language_get();
bool language_is_korean();

// Returns a translated UI string, or the input string when no translation
// exists.  Narrative text is handled by text.cpp/text_ko.txt.
const char *language_text(const char *english);

#endif
