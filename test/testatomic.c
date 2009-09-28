#include "SDL.h"

/*
  Absolutely basic tests just to see if we get the expected value
  after calling each function.
*/

char * 
tf(SDL_bool tf)
{
   static char *t = "true";
   static char *f = "false";

   if (tf)
   {
      return t;
   }

   return f;
}
  
int
main(int argc, char *argv[])
{

   volatile Uint32 val32 = 0;
   Uint32 ret32 = 0;

   volatile Uint64 val64 = 0;
   Uint64 ret64 = 0;

   SDL_SpinLock lock = 0;

   SDL_bool tfret = SDL_FALSE;

   printf("\nspin lock---------------------------------------\n\n");

   SDL_AtomicLock(&lock);
   printf("AtomicLock                   lock=%d\n", lock);
   SDL_AtomicUnlock(&lock);
   printf("AtomicUnlock                 lock=%d\n", lock);

   printf("\n32 bit -----------------------------------------\n\n");

   val32 = 0;
   tfret = SDL_AtomicTestThenSet32(&val32);
   printf("TestThenSet32        tfret=%s val=%d\n", tf(tfret), val32);
   tfret = SDL_AtomicTestThenSet32(&val32);
   printf("TestThenSet32        tfret=%s val=%d\n", tf(tfret), val32);

   SDL_AtomicClear32(&val32);
   printf("Clear32              val=%d\n", val32);

   ret32 = SDL_AtomicFetchThenIncrement32(&val32);
   printf("FetchThenIncrement32 ret=%d val=%d\n", ret32, val32);

   ret32 = SDL_AtomicFetchThenDecrement32(&val32);
   printf("FetchThenDecrement32 ret=%d val=%d\n", ret32, val32);

   ret32 = SDL_AtomicFetchThenAdd32(&val32, 10);
   printf("FetchThenAdd32       ret=%d val=%d\n", ret32, val32);

   ret32 = SDL_AtomicFetchThenSubtract32(&val32, 10);
   printf("FetchThenSubtract32  ret=%d val=%d\n", ret32, val32);

   ret32 = SDL_AtomicIncrementThenFetch32(&val32);
   printf("IncrementThenFetch32 ret=%d val=%d\n", ret32, val32);

   ret32 = SDL_AtomicDecrementThenFetch32(&val32);
   printf("DecrementThenFetch32 ret=%d val=%d\n", ret32, val32);

   ret32 = SDL_AtomicAddThenFetch32(&val32, 10);
   printf("AddThenFetch32       ret=%d val=%d\n", ret32, val32);

   ret32 = SDL_AtomicSubtractThenFetch32(&val32, 10);
   printf("SubtractThenFetch32  ret=%d val=%d\n", ret32, val32);

#ifdef SDL_HAS_64BIT_TYPE
   printf("\n64 bit -----------------------------------------\n\n");

   val64 = 0;
   tfret = SDL_AtomicTestThenSet64(&val64);
   printf("TestThenSet64        tfret=%s val=%llu\n", tf(tfret), ((unsigned long long)val64));
   tfret = SDL_AtomicTestThenSet64(&val64);
   printf("TestThenSet64        tfret=%s val=%llu\n", tf(tfret), ((unsigned long long)val64));

   SDL_AtomicClear64(&val64);
   printf("Clear64              val=%llu\n", ((unsigned long long)val64));

   ret64 = SDL_AtomicFetchThenIncrement64(&val64);
   printf("FetchThenIncrement64 ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));

   ret64 = SDL_AtomicFetchThenDecrement64(&val64);
   printf("FetchThenDecrement64 ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));

   ret64 = SDL_AtomicFetchThenAdd64(&val64, 10);
   printf("FetchThenAdd64       ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));

   ret64 = SDL_AtomicFetchThenSubtract64(&val64, 10);
   printf("FetchThenSubtract64  ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));

   ret64 = SDL_AtomicIncrementThenFetch64(&val64);
   printf("IncrementThenFetch64 ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));

   ret64 = SDL_AtomicDecrementThenFetch64(&val64);
   printf("DecrementThenFetch64 ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));

   ret64 = SDL_AtomicAddThenFetch64(&val64, 10);
   printf("AddThenFetch64       ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));

   ret64 = SDL_AtomicSubtractThenFetch64(&val64, 10);
   printf("SubtractThenFetch64  ret=%llu val=%llu\n", ((unsigned long long)ret64), ((unsigned long long)val64));
#endif

   return 0;
   }
