/****************************************************************************
*
*                   SciTech OS Portability Manager Library
*
*  ========================================================================
*
*   Copyright (C) 1991-2002 SciTech Software, Inc. All rights reserved.
*
*   This file may be distributed and/or modified under the terms of the
*   GNU Lesser General Public License version 2.1 as published by the Free
*   Software Foundation and appearing in the file LICENSE.LGPL included
*   in the packaging of this file.
*
*   Licensees holding a valid Commercial License for this product from
*   SciTech Software, Inc. may use this file in accordance with the
*   Commercial License Agreement provided with the Software.
*
*   This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
*   THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
*   PURPOSE.
*
*   See http://www.scitechsoft.com/license/ for information about
*   the licensing options available and how to purchase a Commercial
*   License Agreement.
*
*   Contact license@scitechsoft.com if any conditions of this licensing
*   are not clear to you, or you have questions about licensing options.
*
*  ========================================================================
*
* Language:     ANSI C
* Environment:  Any
*
* Description:  Header file for PM library functions for querying the CPU
*               type, CPU speed and CPU features. Includes support for
*               high precision timing on Pentium based systems using the
*               Read Time Stamp Counter.
*
****************************************************************************/

#ifndef __CPUINFO_H
#define __CPUINFO_H

//#include "scitech.h"
#include "SDL.h"
#ifdef USE_ASMBLIT
#define __INTEL__
#endif
typedef enum {
	false,
	true
} ibool;
typedef Uint8 uchar;
typedef Uint16 ushort;
typedef Uint32 uint;
typedef Uint32 ulong;
typedef Uint64 u64;
#define _ASMAPI SDLCALL

/*--------------------- Macros and type definitions -----------------------*/

/* Define the calling conventions - C always */

#define ZAPI    _ASMAPI

/****************************************************************************
REMARKS:
Defines the types of processors returned by CPU_getProcessorType.

HEADER:
cpuinfo.h

MEMBERS:
CPU_i386            - Intel 80386 processor
CPU_i486            - Intel 80486 processor
CPU_Pentium         - Intel Pentium(R) processor
CPU_PentiumPro      - Intel PentiumPro(R) processor
CPU_PentiumII       - Intel PentiumII(R) processor
CPU_Celeron         - Intel Celeron(R) processor
CPU_PentiumIII      - Intel PentiumIII(R) processor
CPU_Pentium4        - Intel Pentium4(R) processor
CPU_UnkIntel        - Unknown Intel processor
CPU_Cyrix6x86       - Cyrix 6x86 processor
CPU_Cyrix6x86MX     - Cyrix 6x86MX processor
CPU_CyrixMediaGX    - Cyrix MediaGX processor
CPU_CyrixMediaGXm   - Cyrix MediaGXm processor
CPU_UnkCyrix        - Unknown Cyrix processor
CPU_AMDAm486        - AMD Am486 processor
CPU_AMDAm5x86       - AMD Am5x86 processor
CPU_AMDK5           - AMD K5 processor
CPU_AMDK6           - AMD K6 processor
CPU_AMDK6_2         - AMD K6-2 processor
CPU_AMDK6_2plus     - AMD K6-2+ processor
CPU_AMDK6_III       - AMD K6-III processor
CPU_AMDK6_IIIplus   - AMD K6-III+ processor
CPU_AMDAthlon       - AMD Athlon processor
CPU_AMDDuron        - AMD Duron processor
CPU_UnkAMD          - Unknown AMD processor
CPU_WinChipC6       - IDT WinChip C6 processor
CPU_WinChip2        - IDT WinChip 2 processor
CPU_UnkIDT          - Unknown IDT processor
CPU_ViaCyrixIII     - Via Cyrix III
CPU_UnkVIA          - Unknown Via processor
CPU_Alpha           - DEC Alpha processor
CPU_Mips            - MIPS processor
CPU_PowerPC         - PowerPC processor
CPU_mask            - Mask to remove flags and get CPU type
CPU_IDT             - This bit is set if the processor vendor is IDT
CPU_Cyrix           - This bit is set if the processor vendor is Cyrix
CPU_AMD             - This bit is set if the processor vendor is AMD
CPU_Intel           - This bit is set if the processor vendor is Intel
CPU_VIA             - This bit is set if the processor vendor is Via
CPU_familyMask      - Mask to isolate CPU family
CPU_steppingMask    - Mask to isolate CPU stepping
CPU_steppingShift   - Shift factor for CPU stepping
****************************************************************************/
typedef enum {
    CPU_i386            = 0,
    CPU_i486            = 1,
    CPU_Pentium         = 2,
    CPU_PentiumPro      = 3,
    CPU_PentiumII       = 4,
    CPU_Celeron         = 5,
    CPU_PentiumIII      = 6,
    CPU_Pentium4        = 7,
    CPU_UnkIntel        = 8,
    CPU_Cyrix6x86       = 100,
    CPU_Cyrix6x86MX     = 101,
    CPU_CyrixMediaGX    = 102,
    CPU_CyrixMediaGXm   = 104,
    CPU_UnkCyrix        = 105,
    CPU_AMDAm486        = 200,
    CPU_AMDAm5x86       = 201,
    CPU_AMDK5           = 202,
    CPU_AMDK6           = 203,
    CPU_AMDK6_2         = 204,
    CPU_AMDK6_2plus     = 205,
    CPU_AMDK6_III       = 206,
    CPU_AMDK6_IIIplus   = 207,
    CPU_UnkAMD          = 208,
    CPU_AMDAthlon       = 250,
    CPU_AMDDuron        = 251,
    CPU_WinChipC6       = 300,
    CPU_WinChip2        = 301,
    CPU_UnkIDT          = 302,
    CPU_ViaCyrixIII     = 400,
    CPU_UnkVIA          = 401,
    CPU_Alpha           = 500,
    CPU_Mips            = 600,
    CPU_PowerPC         = 700,
    CPU_mask            = 0x00000FFF,
    CPU_IDT             = 0x00001000,
    CPU_Cyrix           = 0x00002000,
    CPU_AMD             = 0x00004000,
    CPU_Intel           = 0x00008000,
    CPU_VIA             = 0x00010000,
    CPU_familyMask      = 0x00FFF000,
    CPU_steppingMask    = 0x0F000000,
    CPU_steppingShift   = 24
    } CPU_processorType;

#pragma pack(1)
/****************************************************************************
REMARKS:
Defines the structure for holding 64-bit integers used for storing the values
returned by the Intel RDTSC instruction.

HEADER:
cpuinfo.h

MEMBERS:
low     - Low 32-bits of the 64-bit integer
high    - High 32-bits of the 64-bit integer
****************************************************************************/
typedef struct {
    ulong   low;
    ulong   high;
    } CPU_largeInteger;
#pragma pack()

/*-------------------------- Function Prototypes --------------------------*/

#ifdef  __cplusplus
extern "C" {            /* Use "C" linkage when in C++ mode */
#endif

/* Routines to obtain CPU information */

uint    ZAPI CPU_getProcessorType(void);
ibool   ZAPI CPU_haveMMX(void);
ibool   ZAPI CPU_have3DNow(void);
ibool   ZAPI CPU_haveSSE(void);
ibool   ZAPI CPU_haveRDTSC(void);
ulong   ZAPI CPU_getProcessorSpeed(ibool accurate);
void    ZAPI CPU_getProcessorSpeedInHZ(ibool accurate,CPU_largeInteger *speed);
char *  ZAPI CPU_getProcessorName(void);

#ifdef  __cplusplus
}                       /* End of "C" linkage for C++   */
#endif

#endif  /* __CPUINFO_H */

