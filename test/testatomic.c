#include <stdio.h>

#include "SDL.h"
#include "SDL_atomic.h"
#include "SDL_assert.h"

/*
  Absolutely basic tests just to see if we get the expected value
  after calling each function.
*/

static
char *
tf(SDL_bool tf)
{
    static char *t = "TRUE";
    static char *f = "FALSE";

    if (tf)
    {
       return t;
    }

    return f;
}

static
void RunBasicTest()
{
    int value;
    SDL_SpinLock lock = 0;

    SDL_atomic_t v;
    SDL_bool tfret = SDL_FALSE;

    printf("\nspin lock---------------------------------------\n\n");

    SDL_AtomicLock(&lock);
    printf("AtomicLock                   lock=%d\n", lock);
    SDL_AtomicUnlock(&lock);
    printf("AtomicUnlock                 lock=%d\n", lock);

    printf("\natomic -----------------------------------------\n\n");
     
    SDL_AtomicSet(&v, 0);
    tfret = SDL_AtomicSet(&v, 10) == 0;
    printf("AtomicSet(10)        tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));
    tfret = SDL_AtomicAdd(&v, 10) == 10;
    printf("AtomicAdd(10)        tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));

    SDL_AtomicSet(&v, 0);
    SDL_AtomicIncRef(&v);
    tfret = (SDL_AtomicGet(&v) == 1);
    printf("AtomicIncRef()       tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));
    SDL_AtomicIncRef(&v);
    tfret = (SDL_AtomicGet(&v) == 2);
    printf("AtomicIncRef()       tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));
    tfret = (SDL_AtomicDecRef(&v) == SDL_FALSE);
    printf("AtomicDecRef()       tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));
    tfret = (SDL_AtomicDecRef(&v) == SDL_TRUE);
    printf("AtomicDecRef()       tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));

    SDL_AtomicSet(&v, 10);
    tfret = (SDL_AtomicCAS(&v, 0, 20) == SDL_FALSE);
    printf("AtomicCAS()          tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));
    value = SDL_AtomicGet(&v);
    tfret = (SDL_AtomicCAS(&v, value, 20) == SDL_TRUE);
    printf("AtomicCAS()          tfret=%s val=%d\n", tf(tfret), SDL_AtomicGet(&v));
}

/* Atomic operation test, adapted from code by Michael Davidsaver at:
    http://bazaar.launchpad.net/~mdavidsaver/epics-base/atomic/revision/12105#src/libCom/test/epicsAtomicTest.c
*/

/* Tests semantics of atomic operations.  Also a stress test
 * to see if they are really atomic.
 *
 * Serveral threads adding to the same variable.
 * at the end the value is compared with the expected
 * and with a non-atomic counter.
 */
 
/* Number of concurrent incrementers */
#define NThreads 2
#define CountInc 100
#define VALBITS (sizeof(atomicValue)*8)
 
#define atomicValue int
#define CountTo ((atomicValue)((unsigned int)(1<<(VALBITS-1))-1))
#define NInter (CountTo/CountInc/NThreads)
#define Expect (CountTo-NInter*CountInc*NThreads)
 
SDL_COMPILE_TIME_ASSERT(size, CountTo>0); /* check for rollover */
 
static SDL_atomic_t good = { 42 };
 
static atomicValue bad = 42;
 
static SDL_atomic_t threadsRunning;

static SDL_sem *threadDone;
 
static
int adder(void* junk)
{
    unsigned long N=NInter;
    printf("Thread subtracting %d %lu times\n",CountInc,N);
    while (N--) {
        SDL_AtomicAdd(&good, -CountInc);
        bad-=CountInc;
    }
    SDL_AtomicAdd(&threadsRunning, -1);
    SDL_SemPost(threadDone);
    return 0;
}
 
static
void runAdder(void)
{
    Uint32 start, end;
    int T=NThreads;
 
    start = SDL_GetTicks();
 
    threadDone = SDL_CreateSemaphore(0);

    SDL_AtomicSet(&threadsRunning, NThreads);

    while (T--)
        SDL_CreateThread(adder, NULL);
 
    while (SDL_AtomicGet(&threadsRunning) > 0)
        SDL_SemWait(threadDone);
 
    SDL_DestroySemaphore(threadDone);

    end = SDL_GetTicks();
 
    printf("Finished in %f sec\n", (end - start) / 1000.f);
}
 
static
void RunEpicTest()
{
    int b;
    atomicValue v;
 
    printf("\nepic test---------------------------------------\n\n");

    printf("Size asserted to be >= 32-bit\n");
    SDL_assert(sizeof(atomicValue)>=4);
 
    printf("Check static initializer\n");
    v=SDL_AtomicGet(&good);
    SDL_assert(v==42);
 
    SDL_assert(bad==42);
 
    printf("Test negative values\n");
    SDL_AtomicSet(&good, -5);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==-5);
 
    printf("Verify maximum value\n");
    SDL_AtomicSet(&good, CountTo);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==CountTo);
 
    printf("Test compare and exchange\n");
 
    b=SDL_AtomicCAS(&good, 500, 43);
    SDL_assert(!b); /* no swap since CountTo!=500 */
    v=SDL_AtomicGet(&good);
    SDL_assert(v==CountTo); /* ensure no swap */
 
    b=SDL_AtomicCAS(&good, CountTo, 44);
    SDL_assert(!!b); /* will swap */
    v=SDL_AtomicGet(&good);
    SDL_assert(v==44);
 
    printf("Test Add\n");
 
    v=SDL_AtomicAdd(&good, 1);
    SDL_assert(v==44);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==45);
 
    v=SDL_AtomicAdd(&good, 10);
    SDL_assert(v==45);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==55);
 
    printf("Test Add (Negative values)\n");
 
    v=SDL_AtomicAdd(&good, -20);
    SDL_assert(v==55);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==35);
 
    v=SDL_AtomicAdd(&good, -50); /* crossing zero down */
    SDL_assert(v==35);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==-15);
 
    v=SDL_AtomicAdd(&good, 30); /* crossing zero up */
    SDL_assert(v==-15);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==15);
 
    printf("Reset before count down test\n");
    SDL_AtomicSet(&good, CountTo);
    v=SDL_AtomicGet(&good);
    SDL_assert(v==CountTo);
 
    bad=CountTo;
    SDL_assert(bad==CountTo);
 
    printf("Counting down from %d, Expect %d remaining\n",CountTo,Expect);
    runAdder();
 
    v=SDL_AtomicGet(&good);
    printf("Atomic %d Non-Atomic %d\n",v,bad);
    SDL_assert(v==Expect);
    SDL_assert(bad!=Expect);
}

int
main(int argc, char *argv[])
{
    RunBasicTest();
    RunEpicTest();
    return 0;
}
