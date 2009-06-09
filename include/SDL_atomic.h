/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
 */

/**
 * \file SDL_atomic.h
 *
 * Atomic int and pointer magic
 */

#ifndef _SDL_atomic_h_
#define _SDL_atomic_h_


#include "SDL_stdinc.h"
#include "SDL_platform.h"

#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#if defined(__GNUC__) && (defined(i386) || defined(__i386__)  || defined(__x86_64__))
static __inline__ void
SDL_atomic_int_add(volatile int* atomic, int value)
{
  __asm__ __volatile__("lock;"
                       "addl %1, %0"
                       : "=m" (*atomic)
                       : "ir" (value),
                         "m" (*atomic));
}

static __inline__ int
SDL_atomic_int_xchg_add(volatile int* atomic, int value)
{                                              
  int rv;                                    
  __asm__ __volatile__("lock;"               
                       "xaddl %0, %1"        
                       : "=r" (rv),          
                         "=m" (*atomic)    
                       : "0" (value),        
                         "m" (*atomic));   
  return rv;                                        
}

static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{
  int rv;                                                      
  __asm__ __volatile__("lock;"                               
                       "cmpxchgl %2, %1"                     
                       : "=a" (rv),                          
                         "=m" (*atomic)             
                       : "r" (newvalue),                     
                         "m" (*atomic),                    
                         "0" (oldvalue));
  return (rv == oldvalue);                                          
}

static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  void* rv;
  __asm__ __volatile__("lock;"
# if defined(__x86_64__)                       
                       "cmpxchgq %q2, %1"
# else
                       "cmpxchgl %2, %1"
# endif                       
                       : "=a" (rv),
                         "=m" (*atomic)
                       : "r" (newvalue),
                         "m" (*atomic),
                         "0" (oldvalue));
  return (rv == oldvalue);
}
#elif defined(__GNUC__) && defined(__alpha__)
# define ATOMIC_MEMORY_BARRIER (__asm__ __volatile__ ("mb" : : : "memory"))
# define ATOMIC_INT_CMP_XCHG(atomic,value)              \
  ({                                                    \
    int rv,prev;                                        \
    __asm__ __volatile__("   mb\n"                      \
                         "1: ldl_l   %0,%2\n"           \
                         "   cmpeq   %0,%3,%1\n"        \
                         "   beq     %1,2f\n"           \
                         "   mov     %4,%1\n"           \
                         "   stl_c   %1,%2\n"           \
                         "   beq     %1,1b\n"           \
                         "   mb\n"                      \
                         "2:"                           \
                         : "=&r" (prev),                \
                           "=&r" (rv)                   \
                         : "m" (*(atomic)),             \
                           "Ir" (oldvalue),             \
                           "Ir" (newvalue)              \
                         : "memory");                   \
    (rv != 0);                                          \
  })

# if (SIZEOF_VOIDP == 4)
static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  int rv;
  void* prev;
  __asm__ __volatile__("   mb\n"
                       "1: ldl_l %0,%2\n"
                       "   cmpeq %0,%3,%1\n"
                       "   beq   $1,2f\n"
                       "   mov   %4,%1\n"
                       "   stl_c %1,%2\n"
                       "   beq   %1,1b\n"
                       "   mb\n"
                       "2:"
                       : "=&r" (prev),
                         "=&r" (rv)
                       : "m" (*atomic),
                         "Ir" (oldvalue),
                         "Ir" (newvalue)
                       : "memory");
  return (rv != 0);
}
# elif (SIZEOF_VOIDP == 8)
static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  int rv;
  void* prev;
  __asm__ __volatile__("   mb\n"
                       "1: ldq_l %0,%2\n"
                       "   cmpeq %0,%3,%1\n"
                       "   beq   %1,2f\n"
                       "   mov   %4,%1\n"
                       "   stq_c %1,%2\n"
                       "   beq   %1,1b\n"
                       "   mb\n"
                       "2:"
                       : "=&r" (prev),
                         "=&r" (rv)
                       : "m" (*atomic),
                         "Ir" (oldvalue),
                         "Ir" (newvalue)
                       : "memory");
  return (rv != 0);
}
# else
#  error "Your system has an unsupported pointer size"  
# endif  /* SIZEOF_VOIDP */
#elif defined(__GNUC__) && defined(__sparc__)
# define ATOMIC_MEMORY_BARRIER                                          \
  (__asm__ __volatile__("membar #LoadLoad | #LoadStore"                 \
                        " | #StoreLoad | #StoreStore" : : : "memory"))
# define ATOMIC_INT_CMP_XCHG(atomic,oldvalue,newvalue)                  \
  ({                                                                    \
    int rv;                                                             \
    __asm__ __volatile__("cas [%4], %2, %0"                             \
                         : "=r" (rv), "=m" (*(atomic))                  \
                         : "r" (oldvalue), "m" (*(atomic)),             \
                         "r" (atomic), "0" (newvalue));                 \
    rv == oldvalue;                                                     \
  })

# if (SIZEOF_VOIDP == 4)
static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  void* rv;
  __asm__ __volatile__("cas [%4], %2, %0"
                       : "=r" (rv),
                         "=m" (*atomic)
                       : "r" (oldvalue),
                         "m" (*atomic),
                         "r" (atomic),
                         "0" (newvalue));
  return (rv == oldvalue);
}
# elif (SIZEOF_VOIDP == 8)
static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  void* rv;
  void** a = atomic;
  __asm__ __volatile__("casx [%4], %2, %0"
                       : "=r" (rv),
                         "=m" (*a)
                       : "r" (oldvalue),
                         "m" (*a),
                         "r" (a),
                         "0" (newvalue));
  return (rv == oldvalue);
}
# else
#  error "Your system has an unsupported pointer size"
# endif /* SIZEOF_VOIDP */
#elif defined(__GNUC__) && (defined(__POWERPC__) || defined(__powerpc__) || defined(__ppc__) || defined(_M_PPC))
# define ATOMIC_MEMORY_BARRIER \
  (__asm__ __volatile__ ("sync" : : : "memory"))
static __inline__ void
SDL_atomic_int_add(volatile int* atomic, int value)
{                                           
  int rv,tmp;                                   
  __asm__ __volatile__("1: lwarx   %0,  0, %3\n" 
                       "   add     %1, %0, %4\n"
                       "   stwcx.  %1,  0, %3\n" 
                       "   bne-    1b"          
                       : "=&b" (rv),            
                         "=&r" (tmp),           
                         "=m" (*atomic)       
                       : "b" (atomic),          
                         "r" (value),           
                         "m" (*atomic)        
                       : "cr0",                 
                         "memory");             
}

static __inline__ int
SDL_atomic_int_xchg_add(volatile int* atomic, int value)
{                                          
  int rv,tmp;                               
  __asm__ __volatile__("1: lwarx  %0, 0, %3\n"        
                       "   add    %1, %0, %4\n"       
                       "   stwcx. %1, 0, %3\n"        
                       "   bne-   1b"                 
                       : "=&b" (rv),                  
                         "=&r" (tmp),                 
                         "=m" (*atomic)
                       : "b" (atomic),                
                         "r" (value),                 
                         "m" (*atomic)
                       : "cr0",                       
                       "memory");                   
  return rv;                                                 
}

# if (SIZEOF_VOIDP == 4)
static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{                                                        
  int rv;                                                 
  __asm__ __volatile__("   sync\n"                         
                       "1: lwarx   %0, 0, %1\n"           
                       "   subf.   %0, %2, %0\n"          
                       "   bne     2f\n"                  
                       "   stwcx.  %3, 0, %1\n"           
                       "   bne-    1b\n"                  
                       "2: isync"                         
                       : "=&r" (rv)                       
                       : "b" (atomic),                    
                         "r" (oldvalue),                  
                         "r"                              
                       : "cr0",                           
                         "memory");                         
  return (rv == 0);                                              
}

static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  void* rv;
  __asm__ __volatile__("sync\n"
                       "1: lwarx  %0,  0, %1\n"
                       "   subf.  %0, %2, %0\n"
                       "   bne    2f\n"
                       "   stwcx. %3,  0, %1\n"
                       "   bne-   1b\n"
                       "2: isync"
                       : "=&r" (rv)
                       : "b" (atomic),
                         "r" (oldvalue),
                         "r" (newvalue)
                       : "cr0",
                       "memory");
  return (rv == 0);
}
# elif (SIZEOF_VOIDP == 8)
static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{                                                        
  int rv;                                                 
  __asm__ __volatile__("   sync\n"                         
                       "1: lwarx   %0,  0, %1\n"
                       "   extsw   %0, %0\n"
                       "   subf.   %0, %2, %0\n"          
                       "   bne     2f\n"                  
                       "   stwcx.  %3,  0, %1\n"           
                       "   bne-    1b\n"                  
                       "2: isync"                         
                       : "=&r" (rv)                       
                       : "b" (atomic),                    
                         "r" (oldvalue),                  
                         "r"                              
                       : "cr0",                           
                         "memory");                         
  return (rv == 0);                                              
}

static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  void* rv;
  __asm__ __volatile__("sync\n"
                       "1: ldarx  %0,  0, %1\n"
                       "   subf.  %0, %2, %0\n"
                       "   bne    2f\n"
                       "   stdcx. %3,  0, %1\n"
                       "   bne-   1b\n"
                       "2: isync"
                       : "=&r" (rv)
                       : "b" (atomic),
                         "r" (oldvalue),
                         "r" (newvalue)
                       : "cr0",
                       "memory");
  return (rv == 0);
}
# else
#  error "Your system has an unsupported pointer size"
# endif /* SIZEOF_VOIDP */
#elif defined(__GNUC__) && (defined(__IA64__) || defined(__ia64__))
# define ATOMIC_MEMORY_BARRIER (__sync_synchronize())
# define SDL_atomic_int_xchg_add(atomic, value)     \
  (__sync_fetch_and_add((atomic),(value)))
# define SDL_atomic_int_add(atomic, value)                  \
  ((void)__sync_fetch_and_add((atomic),(value)))
# define SDL_atomic_int_cmp_xchg(atomic,oldvalue,newvalue)  \
  (__sync_bool_compare_and_swap((atomic),(oldvalue),(newvalue)))
# define SDL_atomic_ptr_cmp_xchg(atomic,oldvalue,newvalue)              \
  (__sync_bool_compare_and_swap((long*)(atomic),(long)(oldvalue),(long)(newvalue)))
#elif defined(__GNUC__) && defined(__LINUX__) && (defined(__mips__) || defined(__MIPS__))
static __inline__ int
SDL_atomic_int_xchg_add(volatile int* atomic, int value)
{                                            
  int rv,tmp;                                 
  __asm__ __volatile__("1:              \n"                 
                       ".set  push      \n"         
                       ".set  mips2     \n"        
                       "ll    %0,%3     \n"        
                       "addu  %1,%4,%0  \n"     
                       "sc    %1,%2     \n"        
                       ".set  pop       \n"          
                       "beqz  %1,1b     \n"        
                       : "=&r" (rv),          
                         "=&r" (tmp),         
                         "=m" (*atomic)     
                       : "m" (*atomic),     
                         "r" (value)          
                       : "memory");           
  return rv;                                         
}

static __inline__ void
SDL_atomic_int_add(volatile int* atomic, int value)
{                                           
  int rv;                                    
  __asm__ __volatile__("1:               \n"                
                       ".set  push       \n"        
                       ".set  mips2      \n"       
                       "ll    %0,%2      \n"       
                       "addu  %0,%3,%0   \n"    
                       "sc    %0,%1      \n"       
                       ".set  pop        \n"         
                       "beqz  %0,1b      \n"       
                       : "=&r" (rv),         
                         "=m" (*atomic)    
                       : "m" (*atomic),    
                         "r" (value)         
                       : "memory");          
}

static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{
  int rv;
  __asm__ __volatile__("     .set push        \n"
                       "     .set noat        \n"
                       "     .set mips3       \n"
                       "1:   ll   %0, %2      \n"
                       "     bne  %0, %z3, 2f \n"
                       "     .set mips0       \n"
                       "     move $1, %z4     \n"
                       "     .set mips3       \n"
                       "     sc   $1, %1      \n"
                       "     beqz $1, 1b      \n"
                       "     sync             \n"
                       "2:                    \n"
                       "     .set pop         \n"
                       : "=&r" (rv),
                         "=R" (*atomic)
                       : "R" (*atomic),
                         "Jr" (oldvalue),
                         "Jr" (newvalue)
                       : "memory");
  return (SDL_bool)rv;                  
}

static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{                                                     
  int rv;
  __asm__ __volatile__("     .set push        \n"
                       "     .set noat        \n"
                       "     .set mips3       \n"
# if defined(__mips64)
                       "1:   lld  %0, %2      \n"
# else
                       "1:   ll   %0, %2      \n"
# endif                       
                       "     bne  %0, %z3, 2f \n"
                       "     move $1, %z4     \n"
# if defined(__mips64)
                       "     sc   $1, %1      \n"
# else
                       "     scd  $1, %1      \n"
# endif                       
                       "     beqz $1, 1b      \n"
                       "     sync             \n"
                       "2:                    \n"
                       "     .set pop         \n"
                       : "=&r" (rv),
                         "=R" (*atomic)
                       : "R" (*atomic),
                         "Jr" (oldvalue),
                         "Jr" (newvalue)
                       : "memory");
  return (SDL_bool)rv;                                                  
}
#elif defined(__GNUC__) && defined(__m68k__)
static __inline__ int
SDL_atomic_int_xchg_add(volatile int* atomic, int value)
{                                          
  int rv = *atomic;
  int tmp;
  __asm__ __volatile__("1: move%.l %0,%1    \n"
                       "   add%.l  %2,%1    \n"
                       "   cas%.l  %0,%1,%3 \n"
                       "   jbne    1b       \n"
                       : "=d" (rv),
                         "=&d" (tmp)
                       : "d" (value),
                         "m" (*atomic),
                         "0" (rv)
                       : "memory");
  return rv;
}

static __inline__ void
SDL_atomic_int_add(volatile int* atomic, int value)
{                                           
  __asm__ __volatile__("add%.l %0,%1"        
                       :                     
                       : "id" (value),       
                         "m" (*atomic)
                       : "memory");          
}

static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{                                           
  char rv;                                   
  int readvalue;                             
  __asm__ __volatile__("cas%.l %2,%3,%1\n"   
                       "seq    %0"           
                       : "=dm" (rv),         
                         "=m" (*atomic),   
                         "=d" (readvalue)    
                       : "d" (newvalue),     
                         "m" (*atomic),    
                         "2" (oldvalue));    
    return rv;                                        
}

static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  char rv;                                   
  int readvalue;                             
  __asm__ __volatile__("cas%.l %2,%3,%1\n"   
                       "seq    %0"           
                       : "=dm" (rv),         
                         "=m" (*atomic),   
                         "=d" (readvalue)    
                       : "d" (newvalue),     
                         "m" (*atomic),    
                         "2" (oldvalue));    
    return rv;                                        
}
#elif defined(__GNUC__) && defined(__s390__)
# define ATOMIC_INT_CMP_XCHG(atomic,oldvalue,newvalue)  \
  ({                                                    \
    int rv = oldvalue;                                  \
    __asm__ __volatile__("cs %0, %2, %1"                \
                         : "+d" (rv),                   \
                           "=Q" (*(atomic))             \
                         : "d" (newvalue),              \
                           "m" (*(atomic))              \
                         : "cc");                       \
    rv == oldvalue;                                     \
  })
# if (SIZEOF_VOIDP == 4)
static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  void* rv = oldvalue;
  __asm__ __volatile__("cs %0, %2, %1"
                       : "+d" (rv),
                         "=Q" (*atomic)
                       : "d" (newvalue),
                         "m" (*atomic)
                       : "cc");
  return (rv == oldvalue);
}
# elif (SIZEOF_VOIDP == 8)
static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  void* rv = oldvalue;
  void** a = atomic;
  __asm__ __volatile__("csg %0, %2, %1"
                       : "+d" (rv),
                         "=Q" (*a)
                       : "d" ((long)(newvalue)),
                         "m" (*a)
                       : "cc");
  return (rv == oldvalue);
}
# else
#  error "Your system has an unsupported pointer size"
# endif /* SIZEOF_VOIDP */
#elif defined(__WIN32__)
# include <windows.h>
static __inline__ int
SDL_atomic_int_xchg_add(volatile int* atomic, int value)
{
  return InterlockedExchangeAdd(atomic, value);
}

static __inline__ void
SDL_atomic_int_add(volatile int* atomic, int value)
{
  InterlockedExchangeAdd(atomic, value);
}

# if (WINVER > 0X0400)
static __inline__ SDL_bool
SDL_atmoic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{
  return ((SDL_bool)InterlockedCompareExchangePointer((PVOID*)atomic,
                                                      (PVOID)newvalue,
                                                      (PVOID)oldvalue) == oldvalue);
}


static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  return (InterlockedCompareExchangePointer(atomic, newvalue, oldvalue) == oldvalue);
}
# else /* WINVER <= 0x0400 */
#  if (SIZEOF_VOIDP != 4)
#   error "InterlockedCompareExchangePointer needed"
#  endif

static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{
  return (InterlockedCompareExchange(atomic, newvalue, oldvalue) == oldvalue);
}

static __inline__ SDL_bool
SDL_atomic_ptr_cmp_xchg(volatile void** atomic, void* oldvalue, void* newvalue)
{
  return (InterlockedCompareExchange(atomic, newvalue, oldvalue) == oldvalue);
}
# endif
#else /* when all else fails */
# define SDL_ATOMIC_OPS_NOT_SUPPORTED
# warning "Atomic Ops for this platform not supported!"
static __inline__ int
SDL_atomic_int_xchg_add(volatile int* atomic, int value)
{                                           
  int rv = *atomic;                          
  *(atomic) += value;                        
  return rv;                                        
}

static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{
  return (*atomic == oldvalue) ?  
    ((*atomic = newvalue), SDL_TRUE) : SDL_FALSE;
}

static __inline__ void
SDL_atomic_int_add(volatile int* atomic, int value)
{
  *atomic += value;
}
#endif /* arch & platforms */
  
#ifdef ATOMIC_INT_CMP_XCHG
static __inline__ SDL_bool
SDL_atomic_int_cmp_xchg(volatile int* atomic, int oldvalue, int newvalue)
{
  return ATOMIC_INT_CMP_XCHG(atomic,oldvalue,newvalue);
}

static __inline__ int
SDL_atomic_int_xchg_add(volatile int* atomic, int value)   
{                                                   
  int rv;                                            
  do                                                 
    rv = *atomic;                                
  while(!ATOMIC_INT_CMP_XCHG(atomic,rv,rv+value)); 
  return rv;                                                
}

static __inline__ void
SDL_atomic_int_add(volatile int* atomic, int value)
{
  int rv;
  do
    rv = *atomic;
  while(!ATOMIC_INT_CMP_XCHG(atomic,rv,rv+value));
}
#endif /* ATOMIC_CMP_XCHG */

#ifdef ATOMIC_MEMORY_BARRIER
# define SDL_atomic_int_get(atomic) \
  (ATOMIC_MEMORY_BARRIER,*(atomic))
# define SDL_atomic_int_set(atomic,value) \
  (*(atomic)=value,ATOMIC_MEMORY_BARRIER)
#else
# define SDL_atomic_int_get(atomic) (*(atomic))
# define SDL_atomic_int_set(atomic, newvalue) ((void)(*(atomic) = (newvalue)))
#endif /* MEMORY_BARRIER_NEEDED */

#define SDL_atomic_int_inc(atomic) (SDL_atomic_int_add((atomic),1))
#define SDL_atomic_int_dec_test(atomic) (SDL_atomic_int_xchg_add((atomic),-1) == 1)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#include "close_code.h"

#endif /* _SDL_atomic_h_ */

/* vi: set ts=4 sw=4 expandtab: */
