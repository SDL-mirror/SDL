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
main(int argc, char **argv)
{

   volatile Uint32 val32 = 0;
   Uint32 ret32 = 0;

   volatile Uint64 val64 = 0;
   Uint64 ret64 = 0;

   SDL_bool tfret = SDL_FALSE;

   printf("32 bit -----------------------------------------\n\n");

   ret32 = SDL_AtomicExchange32(&val32, 10);
   printf("Exchange32           ret=%d val=%d\n", ret32, val32);
   ret32 = SDL_AtomicExchange32(&val32, 0);
   printf("Exchange32           ret=%d val=%d\n", ret32, val32);

   val32 = 10;
   tfret = SDL_AtomicCompareThenSet32(&val32, 10, 20);
   printf("CompareThenSet32     tfret=%s val=%d\n", tf(tfret), val32);
   val32 = 10;
   tfret = SDL_AtomicCompareThenSet32(&val32, 0, 20);
   printf("CompareThenSet32     tfret=%s val=%d\n", tf(tfret), val32);

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
   printf("64 bit -----------------------------------------\n\n");

   ret64 = SDL_AtomicExchange64(&val64, 10);
   printf("Exchange64           ret=%lld val=%lld\n", ret64, val64);
   ret64 = SDL_AtomicExchange64(&val64, 0);
   printf("Exchange64           ret=%lld val=%lld\n", ret64, val64);

   val64 = 10;
   tfret = SDL_AtomicCompareThenSet64(&val64, 10, 20);
   printf("CompareThenSet64     tfret=%s val=%lld\n", tf(tfret), val64);
   val64 = 10;
   tfret = SDL_AtomicCompareThenSet64(&val64, 0, 20);
   printf("CompareThenSet64     tfret=%s val=%lld\n", tf(tfret), val64);

   val64 = 0;
   tfret = SDL_AtomicTestThenSet64(&val64);
   printf("TestThenSet64        tfret=%s val=%lld\n", tf(tfret), val64);
   tfret = SDL_AtomicTestThenSet64(&val64);
   printf("TestThenSet64        tfret=%s val=%lld\n", tf(tfret), val64);

   SDL_AtomicClear64(&val64);
   printf("Clear64              val=%lld\n", val64);

   ret64 = SDL_AtomicFetchThenIncrement64(&val64);
   printf("FetchThenIncrement64 ret=%lld val=%lld\n", ret64, val64);

   ret64 = SDL_AtomicFetchThenDecrement64(&val64);
   printf("FetchThenDecrement64 ret=%lld val=%lld\n", ret64, val64);

   ret64 = SDL_AtomicFetchThenAdd64(&val64, 10);
   printf("FetchThenAdd64       ret=%lld val=%lld\n", ret64, val64);

   ret64 = SDL_AtomicFetchThenSubtract64(&val64, 10);
   printf("FetchThenSubtract64  ret=%lld val=%lld\n", ret64, val64);

   ret64 = SDL_AtomicIncrementThenFetch64(&val64);
   printf("IncrementThenFetch64 ret=%lld val=%lld\n", ret64, val64);

   ret64 = SDL_AtomicDecrementThenFetch64(&val64);
   printf("DecrementThenFetch64 ret=%lld val=%lld\n", ret64, val64);

   ret64 = SDL_AtomicAddThenFetch64(&val64, 10);
   printf("AddThenFetch64       ret=%lld val=%lld\n", ret64, val64);

   ret64 = SDL_AtomicSubtractThenFetch64(&val64, 10);
   printf("SubtractThenFetch64  ret=%lld val=%lld\n", ret64, val64);
#endif

   return 0;
   }
