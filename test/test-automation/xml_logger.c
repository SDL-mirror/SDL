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

#ifndef _LOGGER_C
#define _LOGGER_C

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* \todo
 * - Fp to LogGenericOutput
 * - print <?xml version="1.0" encoding="utf-8" ?> as header
 * - use SDL_malloc etc...
 * - nest output /OK
 * - dummy functions for no-xml execution
 * - Make XML (and relevant comparisions) case-insensitive
 */


/*! Function pointer definitions. \todo Move to logger.h */
typedef int (*LogInitFp)(void);
typedef int (*LogCleanUptFp)(void);

/*! Function prototypes */
int LogGenericOutput(char *message);
int XMLLogOutput(const char *tag, const char *fmt, va_list list);

static int xml_enabled = 1;

static int loggingPriority = 0;
static int nestingDepth = 0;

enum Priority {
	VERBOSE,
	DEFAULT,
	SILENT
};


#define TAG_START 0x00000001
#define TAG_END   0x00000002
#define TAG_BOTH  (TAG_START & TAG_END)

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
 */
static int
AddOpenTag(const char *tag)
{
	TagList *openTag = malloc(sizeof(TagList));
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
 */
static int
RemoveOpenTag(const char *tag)
{
	if(openTags == NULL) {
		return 1;
	}

	int retVal = 0;

	// Tag should always be the same as previously opened tag
	// to prevent opening and ending tag mismatch
	if(strcmp(openTags->tag, tag) == 0) {
		TagList *openTag = openTags;
		openTags  = openTags->next;

		free(openTag);
	} else {
		printf("Else activated!");
		retVal = 1;
	}

	return retVal;
}

/*!
 * Goes through the open tag list and checks if
 * given tag is already opened
 *
 * \return 1 is tag is open, 0 if not
 */
static int
IsTagOpen(const char *tag)
{
	int retVal = 0;

	TagList *openTag = NULL;
	for(openTag = openTags; openTag; openTag = openTag->next) {
		if(strcmp(openTag->tag, tag) == 0) {
			retVal = 1;
			break;
		}
	}

	return retVal;
}

/*!
 * Debug function. Prints the contents of the open tags list.
 */
static int
PrintOpenTags()
{
	printf("\nOpen tags:\n");

	TagList *openTag = NULL;
	for(openTag = openTags; openTag; openTag = openTag->next) {
		printf("\ttag: %s\n", openTag->tag);
	}
}

//! \TODO move these to upwards!!
int XMLStartTag(int priority, const char *tag);
int XMLEndTag(int priority, const char *tag);

int
XMLLogInit()
{

	LogGenericOutput("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");

	XMLStartTag(0, "testlog");
}

int
XMLLogCleanUp()
{
	//! \todo do CloseOpenTags() instead
	XMLEndTag(0, "testlog");
}

/*!
 * Forms a valid XML-tag based on the given parameters
 *
 * \param tag XML-tag to create
 * \param tagStyle Do start or end tag, or both.
 * \param message text content of the tags
 */
static char *
Tagify(const char *tag, const int tagStyle, const char *message)
{
	// buffer simplifies the creation of the string
	const int bufferSize = 1024;
	char buffer[bufferSize];
	memset(buffer, 0, bufferSize);

	if(tagStyle & TAG_START) {
		strcat(buffer, "<");
		strcat(buffer, tag);
		strcat(buffer, ">");
	}

	if(message) {
		strcat(buffer, message);
	}

	if(tagStyle & TAG_END) {
		strcat(buffer, "</");
		strcat(buffer, tag);
		strcat(buffer, ">");
	}


	const int size = strlen(buffer) + 1;
	char *newTag = malloc(size * sizeof(char));
	memset(newTag, 0, size * sizeof(char));
	memcpy(newTag, buffer, size);

	return newTag;
}


int
XMLStartTag(int priority, const char *tag)
{
	if(priority < loggingPriority) {
		return 1;
	}

	AddOpenTag(tag);
	char *newTag = Tagify(tag, TAG_START, NULL);
	LogGenericOutput(newTag);
	free(newTag);

	nestingDepth++;
}

int
XMLEndTag(int priority, const char *tag)
{
	/*
	Do it before priority check, so incorrect usage of
	priorities won't mess it up
	*/
	nestingDepth--;

	if(priority < loggingPriority) {
		return 1;
	}

	RemoveOpenTag(tag);

	char *newTag = Tagify(tag, TAG_END, NULL);
	LogGenericOutput(newTag);
	free(newTag);
}

int
XMLTag(int priority, const char *tag, const char *fmt, ...)
{
	if(priority < loggingPriority) {
		return 1;
	}

	const int bufferSize = 1024;
	char buffer[bufferSize];
	memset(buffer, 0, bufferSize);

	va_list list;
	va_start(list, fmt);
    vsnprintf(buffer, bufferSize, fmt, list);
	va_end(list);

	char *newTag = Tagify(tag, TAG_BOTH, buffer);
	LogGenericOutput(newTag);
	free(newTag);
}

//! \TODO Make it changeable by using a function pointer
/*!
 * Prints the given message to stderr. Function adds nesting
 * to the output.
 */
int
LogGenericOutput(char *message)
{
	int depth = nestingDepth;
	while(depth--) {
		fprintf(stderr, " ");
	}

	fprintf(stderr, "%s\n", message);
}


/*!
 * Main for testing the logger
 */
int
main()
{
	LogInitFp LogInit = NULL;
	LogCleanUptFp LogCleanUp = NULL;

	if(xml_enabled) {
		// set logger functions to XML
		LogInit = XMLLogInit;
		LogCleanUp = XMLLogCleanUp;
	} else {
		// set up dummy functions
	}

	LogInit();
	XMLStartTag(0, "hello");
	XMLStartTag(0, "world!");
	XMLEndTag(0, "world!");
	XMLEndTag(0, "hello");

	LogCleanUp();



#if 0
	XMLStartTag("log");
	XMLStartTag("suite");
	XMLStartTag("test");

	XMLEndTag("test");
	XMLEndTag("suite");


	PrintOpenTags();
#endif

	return 0;
}

#endif
