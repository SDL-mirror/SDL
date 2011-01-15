#include <stdio.h>
#include "SDL.h"

/*
  Absolutely basic tests just to see if we get the expected value
  after calling each function.
*/

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

int
main(int argc, char *argv[])
{
    int value;
    SDL_SpinLock lock = 0;

    SDL_bool tfret = SDL_FALSE;

    printf("\nspin lock---------------------------------------\n\n");

    SDL_AtomicLock(&lock);
    printf("AtomicLock                   lock=%d\n", lock);
    SDL_AtomicUnlock(&lock);
    printf("AtomicUnlock                 lock=%d\n", lock);

    printf("\natomic -----------------------------------------\n\n");

    SDL_atomic_t v;
     
    SDL_AtomicSet(&v, 0);
    tfret = SDL_AtomicSet(&v, 10) == 0;
    printf("AtomicSet(10)        tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));
    tfret = SDL_AtomicAdd(&v, 10) == 10;
    printf("AtomicAdd(10)        tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));

    SDL_AtomicSet(&v, 0);
    SDL_AtomicIncRef(&v);
    tfret = (SDL_AtomicGet(&v) == 1);
    printf("AtomicIncRef()       tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));
    SDL_AtomicIncRef(&v);
    tfret = (SDL_AtomicGet(&v) == 2);
    printf("AtomicIncRef()       tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));
    tfret = (SDL_AtomicDecRef(&v) == SDL_FALSE);
    printf("AtomicDecRef()       tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));
    tfret = (SDL_AtomicDecRef(&v) == SDL_TRUE);
    printf("AtomicDecRef()       tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));

    SDL_AtomicSet(&v, 10);
    tfret = (SDL_AtomicCAS(&v, 0, 20) != 0);
    printf("AtomicCAS()          tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));
    value = SDL_AtomicGet(&v);
    tfret = (SDL_AtomicCAS(&v, value, 20) == value);
    printf("AtomicCAS()          tfret=%s val=%"PRIu32"\n", tf(tfret), SDL_AtomicGet(&v));

    return 0;
}
