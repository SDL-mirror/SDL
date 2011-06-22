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

	Functions to handle creation of XML elements

===================
*/

static const char *root;

/*! Size for xml element buffer */
#define bufferSize 1024
/*! Buffer for storing the xml element under construction */
static char buffer[bufferSize];

void
XMLOpenDocument(const char *rootTag, LogOutputFp log)
{
	assert(log != NULL);
	logger = log;

	logger("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");

	memset(buffer, 0, bufferSize);
	snprintf(buffer, bufferSize, "<%s>", rootTag);
	logger(buffer);

	// add open tag
	AddOpenTag(rootTag);

	root = rootTag; // it's fine, as long as rootTag points to static memory?
}

void
XMLCloseDocument() {
	XMLCloseElement(root);
}

void
XMLOpenElement(const char *tag)
{
	memset(buffer, 0, bufferSize);
	snprintf(buffer, bufferSize, "<%s>", tag);
	logger(buffer);

	AddOpenTag(tag);
}


void
XMLOpenElementWithAttribute(const char *tag, Attribute *attribute)
{
	memset(buffer, 0, bufferSize);
	snprintf(buffer, bufferSize, "<%s %s='%s'>", tag,
			attribute->attribute, attribute->value);
	logger(buffer);

	AddOpenTag(tag);
}

void
XMLAddContent(const char *content)
{
	memset(buffer, 0, bufferSize);
	snprintf(buffer, bufferSize, "%s", content);
	logger(buffer);
}

void
XMLCloseElement(const char *tag)
{
	// Close the open tags with proper nesting. Closes tags until it finds
	// the given tag which is the last tag that will be closed
	TagList *openTag = openTags;
	while(openTag) {
		TagList *temp = openTag->next;

		memset(buffer, 0, bufferSize);
		snprintf(buffer, bufferSize, "<%s>", openTag->tag);
		logger(buffer);

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
}

