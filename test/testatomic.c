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

   volatile Uint8 val8 = 0;
   Uint8 ret8 = 0;

   volatile Uint16 val16 = 0;
   Uint16 ret16 = 0;

   volatile Uint32 val32 = 0;
   Uint32 ret32 = 0;

   volatile Uint64 val64 = 0;
   Uint64 ret64 = 0;

   SDL_bool tfret = SDL_FALSE;


   printf("8 bit -----------------------------------------\n\n");

   ret8 = SDL_AtomicExchange8(&val8, 10);
   printf("Exchange8           ret=%d val=%d\n", ret8, val8);
   ret8 = SDL_AtomicExchange8(&val8, 0);
   printf("Exchange8           ret=%d val=%d\n", ret8, val8);

   val8 = 10;
   tfret = SDL_AtomicCompareThenSet8(&val8, 10, 20);
   printf("CompareThenSet8     tfret=%s val=%d\n", tf(tfret), val8);
   val8 = 10;
   tfret = SDL_AtomicCompareThenSet8(&val8, 0, 20);
   printf("CompareThenSet8     tfret=%s val=%d\n", tf(tfret), val8);

   val8 = 0;
   tfret = SDL_AtomicTestThenSet8(&val8);
   printf("TestThenSet8        tfret=%s val=%d\n", tf(tfret), val8);
   tfret = SDL_AtomicTestThenSet8(&val8);
   printf("TestThenSet8        tfret=%s val=%d\n", tf(tfret), val8);

   SDL_AtomicClear8(&val8);
   printf("Clear8              val=%d\n", val8);

   ret8 = SDL_AtomicFetchThenIncrement8(&val8);
   printf("FetchThenIncrement8 ret=%d val=%d\n", ret8, val8);

   ret8 = SDL_AtomicFetchThenDecrement8(&val8);
   printf("FetchThenDecrement8 ret=%d val=%d\n", ret8, val8);

   ret8 = SDL_AtomicFetchThenAdd8(&val8, 10);
   printf("FetchThenAdd8       ret=%d val=%d\n", ret8, val8);

   ret8 = SDL_AtomicFetchThenSubtract8(&val8, 10);
   printf("FetchThenSubtract8  ret=%d val=%d\n", ret8, val8);

   ret8 = SDL_AtomicIncrementThenFetch8(&val8);
   printf("IncrementThenFetch8 ret=%d val=%d\n", ret8, val8);

   ret8 = SDL_AtomicDecrementThenFetch8(&val8);
   printf("DecrementThenFetch8 ret=%d val=%d\n", ret8, val8);

   ret8 = SDL_AtomicAddThenFetch8(&val8, 10);
   printf("AddThenFetch8       ret=%d val=%d\n", ret8, val8);

   ret8 = SDL_AtomicSubtractThenFetch8(&val8, 10);
   printf("SubtractThenFetch8  ret=%d val=%d\n", ret8, val8);


   printf("16 bit -----------------------------------------\n\n");

   ret16 = SDL_AtomicExchange16(&val16, 10);
   printf("Exchange16           ret=%d val=%d\n", ret16, val16);
   ret16 = SDL_AtomicExchange16(&val16, 0);
   printf("Exchange16           ret=%d val=%d\n", ret16, val16);

   val16 = 10;
   tfret = SDL_AtomicCompareThenSet16(&val16, 10, 20);
   printf("CompareThenSet16     tfret=%s val=%d\n", tf(tfret), val16);
   val16 = 10;
   tfret = SDL_AtomicCompareThenSet16(&val16, 0, 20);
   printf("CompareThenSet16     tfret=%s val=%d\n", tf(tfret), val16);

   val16 = 0;
   tfret = SDL_AtomicTestThenSet16(&val16);
   printf("TestThenSet16        tfret=%s val=%d\n", tf(tfret), val16);
   tfret = SDL_AtomicTestThenSet16(&val16);
   printf("TestThenSet16        tfret=%s val=%d\n", tf(tfret), val16);

   SDL_AtomicClear16(&val16);
   printf("Clear16              val=%d\n", val16);

   ret16 = SDL_AtomicFetchThenIncrement16(&val16);
   printf("FetchThenIncrement16 ret=%d val=%d\n", ret16, val16);

   ret16 = SDL_AtomicFetchThenDecrement16(&val16);
   printf("FetchThenDecrement16 ret=%d val=%d\n", ret16, val16);

   ret16 = SDL_AtomicFetchThenAdd16(&val16, 10);
   printf("FetchThenAdd16       ret=%d val=%d\n", ret16, val16);

   ret16 = SDL_AtomicFetchThenSubtract16(&val16, 10);
   printf("FetchThenSubtract16  ret=%d val=%d\n", ret16, val16);

   ret16 = SDL_AtomicIncrementThenFetch16(&val16);
   printf("IncrementThenFetch16 ret=%d val=%d\n", ret16, val16);

   ret16 = SDL_AtomicDecrementThenFetch16(&val16);
   printf("DecrementThenFetch16 ret=%d val=%d\n", ret16, val16);

   ret16 = SDL_AtomicAddThenFetch16(&val16, 10);
   printf("AddThenFetch16       ret=%d val=%d\n", ret16, val16);

   ret16 = SDL_AtomicSubtractThenFetch16(&val16, 10);
   printf("SubtractThenFetch16  ret=%d val=%d\n", ret16, val16);

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
