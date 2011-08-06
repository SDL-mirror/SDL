/**
 * Original code: automated SDL rect test written by Edgar Simo "bobbens"
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../../include/SDL_test.h"

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "rect_testIntersectRectAndLine", "description", TEST_ENABLED, 0, 0 };


/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, NULL
};

TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/*!
 * \brief Tests SDL_IntersectRectAndLine()
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRectAndLine
 */
int rect_testIntersectRectAndLine (void *arg)
{
    SDL_Rect rect = { 0, 0, 32, 32 };
    int x1, y1;
    int x2, y2;
    SDL_bool clipped;

    x1 = -10;
    y1 = 0;
    x2 = -10;
    y2 = 31;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( !clipped &&
                   x1 == -10 && y1 == 0 && x2 == -10 && y2 == 31,
        "line outside to the left was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = 40;
    y1 = 0;
    x2 = 40;
    y2 = 31;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( !clipped &&
                   x1 == 40 && y1 == 0 && x2 == 40 && y2 == 31,
        "line outside to the right was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = 0;
    y1 = -10;
    x2 = 31;
    y2 = -10;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( !clipped &&
                   x1 == 0 && y1 == -10 && x2 == 31 && y2 == -10,
        "line outside above was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = 0;
    y1 = 40;
    x2 = 31;
    y2 = 40;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( !clipped &&
                   x1 == 0 && y1 == 40 && x2 == 31 && y2 == 40,
        "line outside below was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = 0;
    y1 = 0;
    x2 = 31;
    y2 = 31;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( clipped &&
                   x1 == 0 && y1 == 0 && x2 == 31 && y2 == 31,
        "line fully inside rect was clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = -10;
    y1 = 15;
    x2 = 40;
    y2 = 15;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( clipped &&
                   x1 == 0 && y1 == 15 && x2 == 31 && y2 == 15,
        "horizontal line rect was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = -32;
    y1 = -32;
    x2 = 63;
    y2 = 63;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( clipped &&
                   x1 == 0 && y1 == 0 && x2 == 31 && y2 == 31,
        "diagonal line to lower right was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = 63;
    y1 = 63;
    x2 = -32;
    y2 = -32;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( clipped &&
                   x1 == 31 && y1 == 31 && x2 == 0 && y2 == 0,
        "diagonal line to upper left was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = 63;
    y1 = -32;
    x2 = -32;
    y2 = 63;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( clipped &&
                   x1 == 31 && y1 == 0 && x2 == 0 && y2 == 31,
        "diagonal line to lower left was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);

    x1 = -32;
    y1 = 63;
    x2 = 63;
    y2 = -32;
    clipped = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue( clipped &&
                   x1 == 0 && y1 == 31 && x2 == 31 && y2 == 0,
        "diagonal line to upper right was incorrectly clipped: %d,%d - %d,%d",
        x1, y1, x2, y2);
}
