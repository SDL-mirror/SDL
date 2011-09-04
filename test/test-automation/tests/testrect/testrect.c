/**
 * Original code: automated SDL rect test written by Edgar Simo "bobbens"
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../../include/SDL_test.h"

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "rect_testIntersectRectAndLine", "Tests SDL_IntersectRectAndLine", TEST_ENABLED, 0, 0 };

static const TestCaseReference test2 =
		(TestCaseReference){ "rect_testIntersectRectInside", "Tests SDL_IntersectRect with B fully contained in A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test3 =
		(TestCaseReference){ "rect_testIntersectRectOutside", "Tests SDL_IntersectRect with B fully outside of A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test4 =
		(TestCaseReference){ "rect_testIntersectRectPartial", "Tests SDL_IntersectRect with B partially intersecting A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test5 =
		(TestCaseReference){ "rect_testIntersectRectPoint", "Tests SDL_IntersectRect with 1x1 sized rectangles", TEST_ENABLED, 0, 0 };


/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, &test4, &test5, NULL
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

/*!
 * \brief Tests SDL_IntersectRect() with B fully inside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectInside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // rectB fully contained in rectA
    refRectB.x = 0;
    refRectB.y = 0;
    refRectB.w = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    AssertTrue(intersection, 
        "Incorrect intersection result: expected true, got false intersecting A (%d,%d,%d,%d) with B (%d,%d,%d,%d)",
        rectA.x, rectA.y, rectA.w, rectA.h, 
        rectB.x, rectB.y, rectB.w, rectB.h);
    AssertTrue(rectA.x == refRectA.x && rectA.y == refRectA.y && rectA.w == refRectA.w && rectA.h == refRectA.h,
        "Source rectangle A was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectA.x, rectA.y, rectA.w, rectA.h,
        refRectA.x, refRectA.y, refRectA.w, refRectA.h);
    AssertTrue(rectB.x == refRectB.x && rectB.y == refRectB.y && rectB.w == refRectB.w && rectB.h == refRectB.h,
        "Source rectangle B was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectB.x, rectB.y, rectB.w, rectB.h,
        refRectB.x, refRectB.y, refRectB.w, refRectB.h);
    AssertTrue(result.x == refRectB.x && result.y == refRectB.y && result.w == refRectB.w && result.h == refRectB.h,
        "Intersection of rectangles A and B was incorrectly calculated as: (%d,%d,%d,%d)",
        result.x, result.y, result.w, result.h);
}

/*!
 * \brief Tests SDL_IntersectRect() with B fully outside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectOutside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // rectB fully outside of rectA
    refRectB.x = refRectA.x + refRectA.w + RandomIntegerInRange(1, 10);
    refRectB.y = refRectA.y + refRectA.h + RandomIntegerInRange(1, 10);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    AssertTrue(!intersection, 
        "Incorrect intersection result: expected false, got true intersecting A (%d,%d,%d,%d) with B (%d,%d,%d,%d)\n",
        rectA.x, rectA.y, rectA.w, rectA.h, 
        rectB.x, rectB.y, rectB.w, rectB.h);    
    AssertTrue(rectA.x == refRectA.x && rectA.y == refRectA.y && rectA.w == refRectA.w && rectA.h == refRectA.h,
        "Source rectangle A was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectA.x, rectA.y, rectA.w, rectA.h,
        refRectA.x, refRectA.y, refRectA.w, refRectA.h);
    AssertTrue(rectB.x == refRectB.x && rectB.y == refRectB.y && rectB.w == refRectB.w && rectB.h == refRectB.h,
        "Source rectangle B was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectB.x, rectB.y, rectB.w, rectB.h,
        refRectB.x, refRectB.y, refRectB.w, refRectB.h);
}

/*!
 * \brief Tests SDL_IntersectRect() with B partially intersecting A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectPartial (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // rectB partially contained in rectA
    refRectB.x = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.y = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    AssertTrue(intersection, 
        "Incorrect intersection result: expected true, got false intersecting A (%d,%d,%d,%d) with B (%d,%d,%d,%d)\n",
        rectA.x, rectA.y, rectA.w, rectA.h, 
        rectB.x, rectB.y, rectB.w, rectB.h);
    AssertTrue(rectA.x == refRectA.x && rectA.y == refRectA.y && rectA.w == refRectA.w && rectA.h == refRectA.h,
        "Source rectangle A was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectA.x, rectA.y, rectA.w, rectA.h,
        refRectA.x, refRectA.y, refRectA.w, refRectA.h);
    AssertTrue(rectB.x == refRectB.x && rectB.y == refRectB.y && rectB.w == refRectB.w && rectB.h == refRectB.h,
        "Source rectangle B was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectB.x, rectB.y, rectB.w, rectB.h,
        refRectB.x, refRectB.y, refRectB.w, refRectB.h);
    AssertTrue(result.x == refRectB.x && result.y == refRectB.y && result.w == (refRectA.w - refRectB.x) && result.h == (refRectA.h - refRectB.y),
        "Intersection of rectangles A and B was incorrectly calculated as: (%d,%d,%d,%d)",
        result.x, result.y, result.w, result.h);
}

/*!
 * \brief Tests SDL_IntersectRect() with 1x1 pixel sized rectangles
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectPoint (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 1, 1 };
    SDL_Rect refRectB = { 0, 0, 1, 1 };
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // intersecting pixels
    refRectA.x = RandomIntegerInRange(1, 100);
    refRectA.y = RandomIntegerInRange(1, 100);
    refRectB.x = refRectA.x;
    refRectB.y = refRectA.y;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    AssertTrue(intersection, 
        "Incorrect intersection result: expected true, got false intersecting A (%d,%d,%d,%d) with B (%d,%d,%d,%d)\n",
        rectA.x, rectA.y, rectA.w, rectA.h, 
        rectB.x, rectB.y, rectB.w, rectB.h);
    AssertTrue(rectA.x == refRectA.x && rectA.y == refRectA.y && rectA.w == refRectA.w && rectA.h == refRectA.h,
        "Source rectangle A was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectA.x, rectA.y, rectA.w, rectA.h,
        refRectA.x, refRectA.y, refRectA.w, refRectA.h);
    AssertTrue(rectB.x == refRectB.x && rectB.y == refRectB.y && rectB.w == refRectB.w && rectB.h == refRectB.h,
        "Source rectangle B was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectB.x, rectB.y, rectB.w, rectB.h,
        refRectB.x, refRectB.y, refRectB.w, refRectB.h);
    AssertTrue(result.x == refRectA.x && result.y == refRectA.y && result.w == refRectA.w && result.h == refRectA.h,
        "Intersection of rectangles A and B was incorrectly calculated as: (%d,%d,%d,%d)",
        result.x, result.y, result.w, result.h);

    // non-intersecting pixels case
    refRectB.x++;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    AssertTrue(!intersection, 
        "Incorrect intersection result: expected false, got true intersecting A (%d,%d,%d,%d) with B (%d,%d,%d,%d)\n",
        rectA.x, rectA.y, rectA.w, rectA.h, 
        rectB.x, rectB.y, rectB.w, rectB.h);
    AssertTrue(rectA.x == refRectA.x && rectA.y == refRectA.y && rectA.w == refRectA.w && rectA.h == refRectA.h,
        "Source rectangle A was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectA.x, rectA.y, rectA.w, rectA.h,
        refRectA.x, refRectA.y, refRectA.w, refRectA.h);
    AssertTrue(rectB.x == refRectB.x && rectB.y == refRectB.y && rectB.w == refRectB.w && rectB.h == refRectB.h,
        "Source rectangle B was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectB.x, rectB.y, rectB.w, rectB.h,
        refRectB.x, refRectB.y, refRectB.w, refRectB.h);
}
