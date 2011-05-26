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

#ifndef _TEST_C
#define _TEST_C

#include <stdio.h>

#include <SDL/SDL.h>

#include "SDL_test.h"

TestCaseReference *references[] =  {
		{"hello", "description", 1, 0},
		{"hello2", "description", 1, 0},
		NULL
};


TestCaseReference **QueryTestCaseReferences() {
	return references;
}

void hello(void *arg){
	TestInit();

	const char *revision = SDL_GetRevision();

	printf("Revision is %s\n", revision);
	AssertEquals("will fail", 3, 5);

	TestQuit();
}

void hello2(void *arg) {
	TestInit();

	// why this isn't segfaulting?
	char *msg = "eello";
	msg[0] = 'H';

	TestQuit();
}

void hello3(void *arg) {
	TestInit();
	printf("hello3\n");

	AssertEquals("passes", 3, 3);

	TestQuit();
}

#endif
