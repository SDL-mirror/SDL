#include "SDL.h"

/*
  Absolutely basic test just to see if we get the expected value after
  calling each function.
*/

int
main(int argc, char **argv)
{

   Uint32 val32 = 0;
   Uint32 ret32 = 0;

   Uint64 val64 = 0;
   Uint64 ret64 = 0;

   SDL_bool tfval = SDL_FALSE;

   ret32 = SDL_AtomicExchange32(&val32, 10);
   tfval = SDL_AtomicCompareThenSet32(&val32, 10, 20);
   tfval = SDL_AtomicTestThenSet32(&val32);
   SDL_AtomicClear32(&val32);
   ret32 = SDL_AtomicFetchThenIncrement32(&val32);
   ret32 = SDL_AtomicFetchThenDecrement32(&val32);
   ret32 = SDL_AtomicFetchThenAdd32(&val32, 10);
   ret32 = SDL_AtomicFetchThenSubtract32(&val32, 10);
   ret32 = SDL_AtomicIncrementThenFetch32(&val32);
   ret32 = SDL_AtomicDecrementThenFetch32(&val32);
   ret32 = SDL_AtomicAddThenFetch32(&val32, 10);
   ret32 = SDL_AtomicSubtractThenFetch32(&val32, 10);

/* #ifdef SDL_HAS_64BIT_TYPE */
#if 0

   ret64 = SDL_AtomicExchange64(&val64, 10);
   tfval = SDL_AtomicCompareThenSet64(&val64, 10, 20);
   tfval = SDL_AtomicTestThenSet64(&val64);
   SDL_AtomicClear64(&val64);
   ret64 = SDL_AtomicFetchThenIncrement64(&val64);
   ret64 = SDL_AtomicFetchThenDecrement64(&val64);
   ret64 = SDL_AtomicFetchThenAdd64(&val64, 10);
   ret64 = SDL_AtomicFetchThenSubtract64(&val64, 10);
   ret64 = SDL_AtomicIncrementThenFetch64(&val64);
   ret64 = SDL_AtomicDecrementThenFetch64(&val64);
   ret64 = SDL_AtomicAddThenFetch64(&val64, 10);
   ret64 = SDL_AtomicSubtractThenFetch64(&val64, 10);
#endif

   return 0;
   }
