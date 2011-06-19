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

#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* \todo
 * - Make XML (and relevant comparisons) case-insensitive
 */

static int xml_enabled = 1;

static int loggingPriority = 0;
static int nestingDepth = 0;

/*! Definitions of log priorities */
typedef enum Priority {
	VERBOSE,
	DEFAULT,
} Priority;

/*! Function pointer definitions. \todo Move to logger.h */
typedef int (*LogOutputFp)(char *);

typedef int (*LogInitFp)(LogOutputFp, Priority);
typedef int (*LogCleanUptFp)(void);

typedef int (*StartTagFp)(Priority, const char *);
typedef int (*EndTagFp)(Priority, const char *);
typedef int (*TagFp)(Priority, const char *, const char *, ...);


/*! Function pointer to output function */
static LogOutputFp OutputFp = NULL;


/*! Definitions for tag styles used in Tagify() */
#define TAG_START 0x00000001
#define TAG_END   0x00000002
#define TAG_BOTH  (TAG_START & TAG_END)

/*! Function prototypes \todo move to xml_logger.h */
int XMLStartTag(Priority priority, const char *tag);
int XMLEndTag(Priority priority, const char *tag);

int LogGenericOutput(char *message);


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

/*!
 * Initializes the XML-logger for creating test reports in XML.
 *
 * \return Error code. \todo
 */
int
XMLInit(LogOutputFp logOutputFp, Priority priority)
{
	OutputFp = logOutputFp;
	loggingPriority = priority;

	//! make "doctype" work with priority level?
	OutputFp("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
	XMLStartTag(DEFAULT, "testlog");
}

/*!
 * Cleans up the logger and closes all open XML-tags
 *
 * \return Error code. \todo
 */
int
XMLCleanUp()
{
	// Close the open tags
	TagList *openTag = openTags;
	while(openTag) {
		TagList *temp = openTag->next;
		XMLEndTag(DEFAULT, openTag->tag);
		openTag = temp;
	}
}

/*!
 * Forms a valid XML-tag based on the given parameters
 *
 * \param tag XML-tag to create
 * \param tagStyle Do start or end tag, or both.
 * \param message text content of the tags
 *
 * \return Well-formed XML tag
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


	const int size = SDL_strlen(buffer) + 1;
	char *newTag = SDL_malloc(size * sizeof(char));
	memset(newTag, 0, size * sizeof(char));
	memcpy(newTag, buffer, size);

	return newTag;
}

/*!
 * Creates and outputs an start tag
 *
 * \param priority Priority of the tag
 * \param tag Tag for outputting
 *
 * \return Error code. Non-zero on failure. Zero on success
 */
int
XMLStartTag(Priority priority, const char *tag)
{
	if(priority < loggingPriority) {
		return 1;
	}

	AddOpenTag(tag);
	char *newTag = Tagify(tag, TAG_START, NULL);
	OutputFp(newTag);
	SDL_free(newTag);

	nestingDepth++;
}

/*!
 * Creates and outputs an end tag
 *
 * \param priority Priority of the tag
 * \param tag Tag for outputting
 *
 *  \return Error code. Non-zero on failure. Zero on success
 */
int
XMLEndTag(Priority priority, const char *tag)
{
	/*
	Do it before priority check, so incorrect usage of
	priorities won't mess it up (?)
	*/
	nestingDepth--;

	if(priority < loggingPriority) {
		return 1;
	}

	RemoveOpenTag(tag);

	char *newTag = Tagify(tag, TAG_END, NULL);
	OutputFp(newTag);
	SDL_free(newTag);
}

/*!
 * Creates an XML-tag including start and end tags and text content
 * between them.
 *
 * \param priority Priority of the tag
 * \param tag Tag for outputting
 * \param fmt Text content of tag as variadic parameter list
 *
 *  \return Error code. Non-zero on failure. Zero on success
 */
int
XMLTag(Priority priority, const char *tag, const char *fmt, ...)
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
	//LogGenericOutput(newTag);
	OutputFp(newTag);
	SDL_free(newTag);
}

/*!
 * Prints the given message to stderr. Function adds nesting
 * to the output.
 *
 * \return Possible error value (\todo)
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



/*! Quick Dummy functions for testing non-xml output. \todo put to proper place*/
int DummyInit(LogOutputFp output, Priority priority) {
	return 0;
}
int DummyCleanUp() {
	return 0;
}
int DummyStartTag(Priority priority, const char *tag) {
	return 0;
}
int DummyEndTag(Priority priority, const char *tag) {
	return 0;
}
int DummyTag(Priority priority, const char *tag, const char *fmt, ...) {
	return 0;
}


/*!
 * Main for testing the logger
 */
int
main(int argc, char *argv[])
{
	LogInitFp LogInit = NULL;
	LogCleanUptFp LogCleanUp = NULL;
	StartTagFp StartTag = NULL;
	EndTagFp EndTag = NULL;
	TagFp Tag = NULL;

	if(xml_enabled) {
		// set logger functions to XML
		LogInit = XMLInit;
		LogCleanUp = XMLCleanUp;

		StartTag = XMLStartTag;
		EndTag = XMLEndTag;
		Tag = XMLTag;
	} else {
		// When no XML-output is desired, dummy functions are used
		LogInit = DummyInit;
		LogCleanUp = DummyCleanUp;

		StartTag = DummyStartTag;
		EndTag = DummyEndTag;
		Tag = DummyTag;
	}

	LogInit(LogGenericOutput, VERBOSE);

	StartTag(DEFAULT, "hello");
	StartTag(DEFAULT, "world");
	EndTag(DEFAULT, "world");
	//EndTag(DEFAULT, "hello");

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
