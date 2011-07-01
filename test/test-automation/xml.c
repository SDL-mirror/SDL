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
#include "logger_helpers.h"

/*! Size for xml element buffer */
#define bufferSize 1024
/*! Buffer for storing the xml element under construction */
static char buffer[bufferSize];

/*! Pointer to XML root element's tag */
static const char *root;

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
	if(openTag->tag == NULL) {
		SDL_free(openTag);
		return 1;
	}

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
	if(openTags == NULL || ValidateString(tag) == 0) {
		return 1;
	}

	int retVal = 0;

	const int size = SDL_strlen(tag);
	char *tempTag = SDL_malloc(size);
	strncpy(tempTag, tag, size);

	// Tag should always be the same as previously opened tag
	// It prevents opening and ending tag mismatch
	if(SDL_strncmp(tempTag, tag, size) == 0) {
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
 * \return Newly allocated escaped string
 */
const char *
EscapeString(const char *string)
{
	//const int bufferSize = 4096;
	char buffer[bufferSize];
	memset(buffer, 0, bufferSize);

	// prevents the code doing a 'bus error'
	char *stringBuffer = SDL_malloc(bufferSize);
	if(stringBuffer == NULL) {
		return NULL;
	}
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


/*
===================

	Functions to handle creation of XML elements

===================
*/

char *
XMLOpenDocument(const char *rootTag, const char *xslStyle)
{
	const char *doctype = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";

	//! \todo refactor this mess
	char *style = NULL;
	if(xslStyle) {
		const char *styleStart = "<?xml-stylesheet type=\"text/xsl\" href=\"";
		const char *styleEnd = "\"?>\n";

		const int sizeStyleStart = SDL_strlen(styleStart);
		const int sizeStyleEnd = SDL_strlen(styleEnd);
		const int sizeStyleSheetName = SDL_strlen(xslStyle);

		const int tempSize = sizeStyleStart + sizeStyleEnd + sizeStyleSheetName + 1;
		style = SDL_malloc(tempSize);
		memset(style, 0, tempSize);
		SDL_snprintf(style, tempSize, "%s%s%s", styleStart, xslStyle, styleEnd);
	}

	memset(buffer, 0, bufferSize);
	SDL_snprintf(buffer, bufferSize, "<%s>", rootTag);

	AddOpenTag(rootTag);

	root = rootTag; // it's fine, as long as rootTag points to static memory?

	char *retBuf = NULL;
	if(xslStyle) {
		const int doctypeSize = SDL_strlen(doctype);
		const int styleSize = SDL_strlen(style);
		const int tagSize = SDL_strlen(buffer);

		const int size = doctypeSize + styleSize + tagSize + 1; // extra byte for '\0'
		retBuf = SDL_malloc(size);

		SDL_snprintf(retBuf, size, "%s%s%s", doctype, style, buffer);

		SDL_free(style);
	} else {
		const int doctypeSize = SDL_strlen(doctype);
		const int tagSize = SDL_strlen(buffer);

		const int size = doctypeSize + tagSize + 1; // extra byte for '\0'
		retBuf = SDL_malloc(size);

		SDL_snprintf(retBuf, size, "%s%s", doctype, buffer);
	}

	return retBuf;
}

char *
XMLCloseDocument() {
	return XMLCloseElement(root);
}

char *
XMLOpenElement(const char *tag)
{
	memset(buffer, 0, bufferSize);
	SDL_snprintf(buffer, bufferSize, "<%s>", tag);

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
	if(ValidateString(content) == 0) {
		return NULL;
	}

	const char *escapedContent = EscapeString(content);

	if(SDL_strlen(escapedContent) >= bufferSize) {
		return NULL;
	}

	memset(buffer, 0, bufferSize);
	SDL_snprintf(buffer, bufferSize, "%s", escapedContent);
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
	if(ValidateString(tag) == 0) {
		return NULL;
	}

	int retBufferSize = 150;
	char *ret = SDL_malloc(retBufferSize);
	memset(ret, 0, retBufferSize);

	// \todo check that element we're trying to close is actually open,
	// otherwise it'll cause nesting problems

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
			SDL_snprintf(buffer, bufferSize, "</%s>", tag);
		} else {
			SDL_snprintf(buffer, bufferSize, "</%s>", openTag->tag);
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

