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
//#include <stdlib.h>
#include <string.h>
//#include <stdarg.h>
#include <assert.h>

#include <SDL/SDL.h>

#include "xml.h"

/*! Points the function which handles the output */
static LogOutputFp logger = 0;

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

	openTag->tag = tag; // Should be fine without malloc?
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

	// Tag should always be the same as previously opened tag
	// It prevents opening and ending tag mismatch
	if(SDL_strcmp(openTags->tag, tag) == 0) {
		TagList *openTag = openTags;
		openTags  = openTags->next;

		free(openTag);
	} else {
		printf("Debug | RemoveOpenTag(): open/end tag mismatch");
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

/*
===================

	XML

===================
*/

static int has_open_element = 0;

void
XMLOpenDocument(const char *rootTag, LogOutputFp log)
{
	assert(log != NULL);
	logger = log;

	logger("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");

	size_t size = SDL_strlen(rootTag) + 3 + 1; /* one extra for '\0', '<' and '>' */
	char *buffer = SDL_malloc(size);
	snprintf(buffer, size, "%s%s%s", "<", rootTag, ">");
	logger(buffer);
	SDL_free(buffer);

	// add open tag
	AddOpenTag(rootTag);
}

void
XMLCloseDocument() {
	// Close the open tags with proper nesting
	TagList *openTag = openTags;
	while(openTag) {
		TagList *temp = openTag->next;

		size_t size = SDL_strlen(openTag->tag) + 4 + 1; /* one extra for '\0', '<', '/' and '>' */
		char *buffer = SDL_malloc(size);
		snprintf(buffer, size, "%s%s%s", "</", openTag->tag, ">");
		logger(buffer);
		SDL_free(buffer);

		RemoveOpenTag(openTag->tag);

		openTag = temp;
	}
}

static const char *currentTag = NULL;

void
XMLOpenElement(const char *tag)
{
	size_t size = SDL_strlen(tag) + 2 + 1; /* one extra for '\0', '<' */
	char *buffer = SDL_malloc(size);
	snprintf(buffer, size, "%s%s%s", "<", tag, ">");
	logger(buffer);
	SDL_free(buffer);

	currentTag = tag;

	has_open_element = 1;

	AddOpenTag(tag);
}


void
XMLOpenElementWithAttribute(const char *tag, Attribute attribute)
{
	size_t size = SDL_strlen(tag) + 2 + 1; /* one extra for '\0', '<' */
	char *buffer = SDL_malloc(size);

	snprintf(buffer,  size, "%s%s", "<", tag);
	logger(buffer);
	SDL_free(buffer);

	currentTag = tag;

	has_open_element = 1;

	AddOpenTag(tag);
}

//! \todo make this static and remove from interface?
void
XMLAddAttribute(const char *attribute, const char *value)
{
	// Requires open element
	if(has_open_element == 0) {
		return ;
	}
	size_t attributeSize = SDL_strlen(attribute);
	size_t valueSize = SDL_strlen(value);

	size_t size = 1 + attributeSize + 3 + valueSize + 1;
	char *buffer = SDL_malloc(size); // 1 for '='
	snprintf(buffer, size, " %s%s\"%s\"", attribute, "=", value);
	logger(buffer);
	SDL_free(buffer);
}

void
XMLAddContent(const char *content)
{
	size_t size = SDL_strlen(content) + 1 + 1;
	char *buffer = SDL_malloc(size);
	snprintf(buffer, size, "%s", content);
	logger(buffer);
	SDL_free(buffer);}

void
XMLCloseElement(const char *tag)
{
	// Close the open tags with proper nesting. Closes tags until it finds
	// the given tag which is the last tag that will be closed
	TagList *openTag = openTags;
	while(openTag) {
		TagList *temp = openTag->next;

		size_t size = SDL_strlen(openTag->tag) + 4 + 1; /* one extra for '\0', '<', '/' and '>' */
		char *buffer = SDL_malloc(size);
		snprintf(buffer, size, "%s%s%s", "</", openTag->tag, ">");
		logger(buffer);
		SDL_free(buffer);

		const int openTagSize = SDL_strlen(openTag->tag);
		const int tagSize = SDL_strlen(tag);
		const int compSize = (openTagSize > tagSize) ? openTagSize : tagSize;

		int breakOut = 0;
		if(SDL_strncmp(openTag->tag, tag, compSize) == 0) {
			breakOut = 1;
		}

		RemoveOpenTag(openTag->tag);

		openTag = temp;

		if(breakOut) {
			break;
		}
	}

	has_open_element = 0;
}
