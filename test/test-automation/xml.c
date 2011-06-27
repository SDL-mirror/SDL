/*
  Copyright (C) 2011 Markus Kauppila <markus.kauppila@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <SDL/SDL.h>

#include "xml.h"

/*!
 * Defines structure used for "counting" open XML-tags
 */
typedef struct TagList {
	const char *tag;
	struct TagList *next;
} TagList;

static TagList *openTags = NULL;

/*!
 * Prepend the open tags list
 *
 * \return On error returns non-zero value, otherwise zero will returned
 */
static int
AddOpenTag(const char *tag)
{
	TagList *openTag = SDL_malloc(sizeof(TagList));
	if(openTag == NULL) {
		return 1;
	}
	memset(openTag, 0, sizeof(TagList));

	const int tagSize = SDL_strlen(tag) + 1;
	openTag->tag = SDL_malloc(tagSize);
	strncpy((char *)openTag->tag, (char *)tag, tagSize);

	openTag->next = openTags;

	openTags = openTag;

	return 0;
}

/*!
 * Removes the first tag from the open tag list
 *
 * \return On error returns non-zero value, otherwise zero will returned
 */
static int
RemoveOpenTag(const char *tag)
{
	if(openTags == NULL) {
		return 1;
	}

	int retVal = 0;

	const int size = SDL_strlen(tag);
	char *tempTag = SDL_malloc(size);
	strncpy(tempTag, tag, size);

	// Tag should always be the same as previously opened tag
	// It prevents opening and ending tag mismatch
	if(SDL_strcmp(tempTag, tag) == 0) {
		TagList *openTag = openTags;
		SDL_free((char *)openTag->tag);

		openTags  = openTags->next;
		SDL_free(openTag);
	} else {
		//printf("Debug | xml.c:RemoveOpenTag(): open/end tag mismatch");
		retVal = 1;
	}

	return retVal;
}

/*!
 * Debug function. Prints the contents of the open tags list.
 */
static void
PrintOpenTags()
{
	printf("\nOpen tags:\n");

	TagList *openTag = NULL;
	for(openTag = openTags; openTag; openTag = openTag->next) {
		printf("\ttag: %s\n", openTag->tag);
	}
}


/*!
 * Converts the special characters ', ", <, >, and & to
 * corresponding entities: &apos; &quot; &lt; &gt; and &amp;
 *
 * \param string String to be escaped
 * \return Escaped string
 */
const char *EscapeString(const char *string) {
	const int bufferSize = 4096;
	char buffer[bufferSize];
	memset(buffer, 0, bufferSize);

	// prevents the code doing a 'bus error'
	char *stringBuffer = SDL_malloc(bufferSize);
	strncpy(stringBuffer, string, bufferSize);

	// Ampersand (&) must be first, otherwise it'll mess up the other entities
	char *characters[] = {"&", "'", "\"", "<", ">"};
	char *entities[] = {"&amp;", "&apos;", "&quot;", "&lt;", "&gt;"};
	int maxCount = 5;

	int counter = 0;
	for(; counter < maxCount; ++counter) {
		char *character = characters[counter];
		char *entity = entities[counter];

		if(strstr(stringBuffer, character) == NULL)
			continue;

		char *token = strtok(stringBuffer, character);
		while(token) {
			char *nextToken = strtok(NULL, character);

			int bytesLeft = bufferSize - SDL_strlen(buffer);
			if(bytesLeft) {
				strncat(buffer, token, bytesLeft);
			} else {
				// \! todo there's probably better way to report an error?
				fprintf(stderr, "xml.c | EscapingString: Buffer is full");
			}

			if(nextToken)
				strcat(buffer, entity);

			token = nextToken;
		}

		memcpy(stringBuffer, buffer, bufferSize);
		memset(buffer, 0, bufferSize);
	}

	return stringBuffer;
}

/*! Turns all the characters of the given
 * string to lowercase and returns the resulting string.
 *
 * \param string String to be converted
 * \return Lower-case version of the given string
 */
char *
ToLowerCase(const char *string)
{
	const int size = SDL_strlen(string);
	char *ret = SDL_malloc(size + 1);
	strncpy(ret, string, size);
	ret[size] = '\0';

	int counter = 0;
	for(; counter < size; ++counter) {
		ret[counter] = tolower(ret[counter]);
	}

	// printf("Debug: %s == %s\n", string, ret);

	return ret;
}

/*
===================

	Functions to handle creation of XML elements

===================
*/

static const char *root;

/*! Size for xml element buffer */
#define bufferSize 1024
/*! Buffer for storing the xml element under construction */
static char buffer[bufferSize];

char *
XMLOpenDocument(const char *rootTag)
{
	const char *doctype = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";

	memset(buffer, 0, bufferSize);
	snprintf(buffer, bufferSize, "<%s>", rootTag);

	AddOpenTag(rootTag);

	root = rootTag; // it's fine, as long as rootTag points to static memory?

	const int doctypeSize = SDL_strlen(doctype);
	const int tagSize = SDL_strlen(buffer);

	const int size = doctypeSize + tagSize + 1; // extra byte for '\0'
	char *ret = SDL_malloc(size);
	// copy doctype
	strncpy(ret, doctype, doctypeSize);
	// copy tag
	strncpy(ret + doctypeSize, buffer, tagSize);
	ret[size] = '\0';
	return ret;
}

char *
XMLCloseDocument() {
	return XMLCloseElement(root);
}

char *
XMLOpenElement(const char *tag)
{
	memset(buffer, 0, bufferSize);
	snprintf(buffer, bufferSize, "<%s>", tag);

	AddOpenTag(tag);

	const int size = SDL_strlen(buffer);
	char *ret = SDL_malloc(size + 1);
	strncpy(ret, buffer, size);
	ret[size] = '\0';

	return ret;
}

char *
XMLAddContent(const char *content)
{
	const char *escapedContent = EscapeString(content);

	memset(buffer, 0, bufferSize);
	snprintf(buffer, bufferSize, "%s", escapedContent);
	SDL_free((char *)escapedContent);

	const int size = SDL_strlen(buffer);
	char *ret = SDL_malloc(size + 1);
	strncpy(ret, buffer, size);
	ret[size] = '\0';

	return ret;
}

char *
XMLCloseElement(const char *tag)
{
	char *ret = SDL_malloc(bufferSize);
	memset(ret, 0, bufferSize);

	// \todo check that element we're trying is actually open,
	// otherwise it'll case nesting problems

	// Close the open tags with proper nesting. Closes tags until it finds
	// the given tag which is the last tag that will be closed
	TagList *openTag = openTags;
	while(openTag) {
		TagList *temp = openTag->next;

		char *lowOpenTag = ToLowerCase(openTag->tag);
		char *lowTag = ToLowerCase(tag);

		const int openTagSize = SDL_strlen(lowOpenTag);
		const int tagSize = SDL_strlen(lowTag);
		const int compSize = (openTagSize > tagSize) ? openTagSize : tagSize;

		memset(buffer, 0, bufferSize);

		int breakOut = 0;
		if(SDL_strncmp(lowOpenTag, lowTag, compSize) == 0) {
			breakOut = 1;
			snprintf(buffer, bufferSize, "</%s>", tag);
		} else {
			snprintf(buffer, bufferSize, "</%s>", openTag->tag);
		}

		SDL_free(lowOpenTag);
		SDL_free(lowTag);

		int bytesLeft = bufferSize - SDL_strlen(ret);
		if(bytesLeft) {
			strncat(ret, buffer, bytesLeft);
		} else {
			// \! todo there's probably better way to report an error?
			fprintf(stderr, "xml.c | XMLCloseElement: Buffer is full");
		}

		RemoveOpenTag(openTag->tag);

		openTag = temp;

		if(breakOut) {
			break;
		}
	}

	return ret;
}

