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

static const TestCaseReference test6 =
		(TestCaseReference){ "rect_testIntersectRectParam", "Negative tests against SDL_IntersectRect with invalid parameters", TEST_ENABLED, 0, 0 };

static const TestCaseReference test7 =
		(TestCaseReference){ "rect_testHasIntersectionInside", "Tests SDL_HasIntersection with B fully contained in A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test8 =
		(TestCaseReference){ "rect_testHasIntersectionOutside", "Tests SDL_HasIntersection with B fully outside of A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test9 =
		(TestCaseReference){ "rect_testHasIntersectionPartial", "Tests SDL_HasIntersection with B partially intersecting A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test10 =
		(TestCaseReference){ "rect_testHasIntersectionPoint", "Tests SDL_HasIntersection with 1x1 sized rectangles", TEST_ENABLED, 0, 0 };

static const TestCaseReference test11 =
		(TestCaseReference){ "rect_testHasIntersectionParam", "Negative tests against SDL_HasIntersection with invalid parameters", TEST_ENABLED, 0, 0 };


/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, &test4, &test5, &test6, &test7, &test8, &test9, &test10, &test11, NULL
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
 * \brief Private helper to check SDL_HasIntersection results
 */
void _validateHasIntersectionResults(
    SDL_bool intersection, SDL_bool expectedIntersection, 
    SDL_Rect *rectA, SDL_Rect *rectB, SDL_Rect *refRectA, SDL_Rect *refRectB)
{
    AssertTrue(intersection == expectedIntersection, 
        "Incorrect intersection result: expected %s, got %s intersecting A (%d,%d,%d,%d) with B (%d,%d,%d,%d)\n",
        (expectedIntersection == SDL_TRUE) ? "true" : "false",
        (intersection == SDL_TRUE) ? "true" : "false",
        rectA->x, rectA->y, rectA->w, rectA->h, 
        rectB->x, rectB->y, rectB->w, rectB->h);
    AssertTrue(rectA->x == refRectA->x && rectA->y == refRectA->y && rectA->w == refRectA->w && rectA->h == refRectA->h,
        "Source rectangle A was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectA->x, rectA->y, rectA->w, rectA->h,
        refRectA->x, refRectA->y, refRectA->w, refRectA->h);
    AssertTrue(rectB->x == refRectB->x && rectB->y == refRectB->y && rectB->w == refRectB->w && rectB->h == refRectB->h,
        "Source rectangle B was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectB->x, rectB->y, rectB->w, rectB->h,
        refRectB->x, refRectB->y, refRectB->w, refRectB->h);
}

/*!
 * \brief Private helper to check SDL_IntersectRect results
 */
void _validateIntersectRectResults(
    SDL_bool intersection, SDL_bool expectedIntersection, 
    SDL_Rect *rectA, SDL_Rect *rectB, SDL_Rect *refRectA, SDL_Rect *refRectB, 
    SDL_Rect *result, SDL_Rect *expectedResult)
{
    _validateHasIntersectionResults(intersection, expectedIntersection, rectA, rectB, refRectA, refRectB);
    if (result && expectedResult) {
        AssertTrue(result->x == expectedResult->x && result->y == expectedResult->y && result->w == expectedResult->w && result->h == expectedResult->h,
            "Intersection of rectangles A and B was incorrectly calculated, got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
            result->x, result->y, result->w, result->h,
            expectedResult->x, expectedResult->y, expectedResult->w, expectedResult->h);
    }
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
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &refRectB);
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
    _validateIntersectRectResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB, (SDL_Rect *)NULL, (SDL_Rect *)NULL);    
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
    SDL_Rect expectedResult;
    SDL_bool intersection;

    // rectB partially contained in rectA
    refRectB.x = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.y = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    expectedResult.x = refRectB.x;
    expectedResult.y = refRectB.y;
    expectedResult.w = refRectA.w - refRectB.x;
    expectedResult.h = refRectA.h - refRectB.y;    
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &expectedResult);
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
    int offsetX, offsetY;

    // intersecting pixels
    refRectA.x = RandomIntegerInRange(1, 100);
    refRectA.y = RandomIntegerInRange(1, 100);
    refRectB.x = refRectA.x;
    refRectB.y = refRectA.y;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &refRectA);

    // non-intersecting pixels cases
    for (offsetX = -1; offsetX <= 1; offsetX++) {
        for (offsetY = -1; offsetY <= 1; offsetY++) {
            if (offsetX != 0 || offsetY != 0) {
                refRectA.x = RandomIntegerInRange(1, 100);
                refRectA.y = RandomIntegerInRange(1, 100);
                refRectB.x = refRectA.x;
                refRectB.y = refRectA.y;    
                refRectB.x += offsetX;
                refRectB.y += offsetY;
                rectA = refRectA;
                rectB = refRectB;
                intersection = SDL_IntersectRect(&rectA, &rectB, &result);
                _validateIntersectRectResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB, (SDL_Rect *)NULL, (SDL_Rect *)NULL);
            }
        }
    }
}

/*!
 * \brief Negative tests against SDL_IntersectRect() with invalid parameters
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectParam(void *arg)
{
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // invalid parameter combinations
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, &rectB, &result);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st parameter was NULL"); 
    intersection = SDL_IntersectRect(&rectA, (SDL_Rect *)NULL, &result);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 2st parameter was NULL"); 
    intersection = SDL_IntersectRect(&rectA, &rectB, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 3st parameter was NULL"); 
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, (SDL_Rect *)NULL, &result);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st and 2nd parameters were NULL"); 
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, &rectB, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st and 3rd parameters were NULL "); 
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, (SDL_Rect *)NULL, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when all parameters were NULL");     
}

/*!
 * \brief Tests SDL_HasIntersection() with B fully inside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionInside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // rectB fully contained in rectA
    refRectB.x = 0;
    refRectB.y = 0;
    refRectB.w = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);
}

/*!
 * \brief Tests SDL_HasIntersection() with B fully outside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionOutside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // rectB fully outside of rectA
    refRectB.x = refRectA.x + refRectA.w + RandomIntegerInRange(1, 10);
    refRectB.y = refRectA.y + refRectA.h + RandomIntegerInRange(1, 10);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB);
}

/*!
 * \brief Tests SDL_HasIntersection() with B partially intersecting A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionPartial (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // rectB partially contained in rectA
    refRectB.x = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.y = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);
}

/*!
 * \brief Tests SDL_HasIntersection() with 1x1 pixel sized rectangles
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionPoint (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 1, 1 };
    SDL_Rect refRectB = { 0, 0, 1, 1 };
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;
    int offsetX, offsetY;

    // intersecting pixels
    refRectA.x = RandomIntegerInRange(1, 100);
    refRectA.y = RandomIntegerInRange(1, 100);
    refRectB.x = refRectA.x;
    refRectB.y = refRectA.y;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);

    // non-intersecting pixels cases
    for (offsetX = -1; offsetX <= 1; offsetX++) {
        for (offsetY = -1; offsetY <= 1; offsetY++) {
            if (offsetX != 0 || offsetY != 0) {
                refRectA.x = RandomIntegerInRange(1, 100);
                refRectA.y = RandomIntegerInRange(1, 100);
                refRectB.x = refRectA.x;
                refRectB.y = refRectA.y;    
                refRectB.x += offsetX;
                refRectB.y += offsetY;
                rectA = refRectA;
                rectB = refRectB;
                intersection = SDL_HasIntersection(&rectA, &rectB);
                _validateHasIntersectionResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB);
            }
        }
    }
}

/*!
 * \brief Negative tests against SDL_HasIntersection() with invalid parameters
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionParam(void *arg)
{
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // invalid parameter combinations
    intersection = SDL_HasIntersection((SDL_Rect *)NULL, &rectB);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st parameter was NULL"); 
    intersection = SDL_HasIntersection(&rectA, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 2st parameter was NULL"); 
    intersection = SDL_HasIntersection((SDL_Rect *)NULL, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when all parameters were NULL");     
}
