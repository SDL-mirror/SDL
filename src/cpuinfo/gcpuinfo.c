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
* Description:  Main module to implement the Zen Timer support functions.
*
****************************************************************************/

#include "cpuinfo.h"
//#include "pmapi.h"
//#include "oshdr.h"

/*----------------------------- Implementation ----------------------------*/

/* External Intel assembler functions */
#ifdef  __INTEL__
/* {secret} */
ibool   _ASMAPI _CPU_haveCPUID(void);
/* {secret} */
ibool   _ASMAPI _CPU_check80386(void);
/* {secret} */
ibool   _ASMAPI _CPU_check80486(void);
/* {secret} */
uint    _ASMAPI _CPU_checkCPUID(void);
/* {secret} */
uint    _ASMAPI _CPU_getCPUIDModel(void);
/* {secret} */
uint    _ASMAPI _CPU_getCPUIDStepping(void);
/* {secret} */
uint    _ASMAPI _CPU_getCPUIDFeatures(void);
/* {secret} */
uint    _ASMAPI _CPU_getCacheSize(void);
/* {secret} */
uint    _ASMAPI _CPU_have3DNow(void);
/* {secret} */
ibool   _ASMAPI _CPU_checkClone(void);
/* {secret} */
void    _ASMAPI _CPU_readTimeStamp(CPU_largeInteger *time);
/* {secret} */
void    _ASMAPI _CPU_runBSFLoop(ulong iterations);
/* {secret} */
ulong   _ASMAPI _CPU_mulDiv(ulong a,ulong b,ulong c);
/* {secret} */
#define CPU_HaveMMX     0x00800000
#define CPU_HaveRDTSC   0x00000010
#define CPU_HaveSSE     0x02000000
#endif

/*------------------------ Public interface routines ----------------------*/

#ifdef __INTEL__
extern Uint8 PM_inpb(int port);
extern void PM_outpb(int port,Uint8 val);

/****************************************************************************
REMARKS:
Read an I/O port location.
****************************************************************************/
static uchar rdinx(
    int port,
    int index)
{
    PM_outpb(port,(uchar)index);
    return PM_inpb(port+1);
}

/****************************************************************************
REMARKS:
Write an I/O port location.
****************************************************************************/
static void wrinx(
    ushort port,
    ushort index,
    ushort value)
{
    PM_outpb(port,(uchar)index);
    PM_outpb(port+1,(uchar)value);
}

/****************************************************************************
REMARKS:
Enables the Cyrix CPUID instruction to properly detect MediaGX and 6x86
processors.
****************************************************************************/
static void _CPU_enableCyrixCPUID(void)
{
    uchar   ccr3;

    //PM_init();
    ccr3 = rdinx(0x22,0xC3);
    wrinx(0x22,0xC3,(uchar)(ccr3 | 0x10));
    wrinx(0x22,0xE8,(uchar)(rdinx(0x22,0xE8) | 0x80));
    wrinx(0x22,0xC3,ccr3);
}
#endif

/****************************************************************************
DESCRIPTION:
Returns the type of processor in the system.

HEADER:
cpuinfo.h

RETURNS:
Numerical identifier for the installed processor

REMARKS:
Returns the type of processor in the system. Note that if the CPU is an
unknown Pentium family processor that we don't have an enumeration for,
the return value will be greater than or equal to the value of CPU_UnkPentium
(depending on the value returned by the CPUID instruction).

SEE ALSO:
CPU_getProcessorSpeed, CPU_haveMMX, CPU_getProcessorName
****************************************************************************/
uint ZAPI CPU_getProcessorType(void)
{
#if     defined(__INTEL__)
    uint            cpu,vendor,model,cacheSize;
    static ibool    firstTime = true;

    if (_CPU_haveCPUID()) {
        cpu = _CPU_checkCPUID();
        vendor = cpu & ~CPU_mask;
        if (vendor == CPU_Intel) {
            /* Check for Intel processors */
            switch (cpu & CPU_mask) {
                case 4: cpu = CPU_i486;         break;
                case 5: cpu = CPU_Pentium;      break;
                case 6:
                    if ((model = _CPU_getCPUIDModel()) == 1)
                        cpu = CPU_PentiumPro;
                    else if (model <= 6) {
                        cacheSize = _CPU_getCacheSize();
                        if ((model == 5 && cacheSize == 0) ||
                            (model == 5 && cacheSize == 256) ||
                            (model == 6 && cacheSize == 128))
                            cpu = CPU_Celeron;
                        else
                            cpu = CPU_PentiumII;
                        }
                    else if (model >= 7) {
                        /* Model 7 == Pentium III */
                        /* Model 8 == Celeron/Pentium III Coppermine */
                        cacheSize = _CPU_getCacheSize();
                        if ((model == 8 && cacheSize == 128))
                            cpu = CPU_Celeron;
                        else
                            cpu = CPU_PentiumIII;
                        }
                    break;
                case 7:
                    cpu = CPU_Pentium4;
                    break;
                default:
                    cpu = CPU_UnkIntel;
                    break;
                }
            }
        else if (vendor == CPU_Cyrix) {
            /* Check for Cyrix processors */
            switch (cpu & CPU_mask) {
                case 4:
                    if ((model = _CPU_getCPUIDModel()) == 4)
                        cpu = CPU_CyrixMediaGX;
                    else
                        cpu = CPU_UnkCyrix;
                    break;
                case 5:
                    if ((model = _CPU_getCPUIDModel()) == 2)
                        cpu = CPU_Cyrix6x86;
                    else if (model == 4)
                        cpu = CPU_CyrixMediaGXm;
                    else
                        cpu = CPU_UnkCyrix;
                    break;
                case 6:
                    if ((model = _CPU_getCPUIDModel()) <= 1)
                        cpu = CPU_Cyrix6x86MX;
                    else
                        cpu = CPU_UnkCyrix;
                    break;
                default:
                    cpu = CPU_UnkCyrix;
                    break;
                }
            }
        else if (vendor == CPU_AMD) {
            /* Check for AMD processors */
            switch (cpu & CPU_mask) {
                case 4:
                    if ((model = _CPU_getCPUIDModel()) == 0)
                        cpu = CPU_AMDAm5x86;
                    else
                        cpu = CPU_AMDAm486;
                    break;
                case 5:
                    if ((model = _CPU_getCPUIDModel()) <= 3)
                        cpu = CPU_AMDK5;
                    else if (model <= 7)
                        cpu = CPU_AMDK6;
                    else if (model == 8)
                        cpu = CPU_AMDK6_2;
                    else if (model == 9)
                        cpu = CPU_AMDK6_III;
                    else if (model == 13) {
                        if (_CPU_getCPUIDStepping() <= 3)
                            cpu = CPU_AMDK6_IIIplus;
                        else
                            cpu = CPU_AMDK6_2plus;
                        }
                    else
                        cpu = CPU_UnkAMD;
                    break;
                case 6:
                    if ((model = _CPU_getCPUIDModel()) == 3)
                        cpu = CPU_AMDDuron;
                    else
                        cpu = CPU_AMDAthlon;
                    break;
                default:
                    cpu = CPU_UnkAMD;
                    break;
                }
            }
        else if (vendor == CPU_IDT) {
            /* Check for IDT WinChip processors */
            switch (cpu & CPU_mask) {
                case 5:
                    if ((model = _CPU_getCPUIDModel()) <= 4)
                        cpu = CPU_WinChipC6;
                    else if (model == 8)
                        cpu = CPU_WinChip2;
                    else
                        cpu = CPU_UnkIDT;
                    break;
                case 6:
                    vendor = CPU_VIA;
                    if ((model = _CPU_getCPUIDModel()) <= 6)
                        cpu = CPU_ViaCyrixIII;
                    else
                        cpu = CPU_UnkVIA;
                    break;
                default:
                    vendor = CPU_VIA;
                    cpu = CPU_UnkVIA;
                    break;
                }
            }
        else {
            /* Assume a Pentium compatible Intel clone */
            cpu = CPU_Pentium;
            }
        return cpu | vendor | (_CPU_getCPUIDStepping() << CPU_steppingShift);
        }
    else {
        if (_CPU_check80386())
            cpu = CPU_i386;
        else  if (_CPU_check80486()) {
            /* If we get here we may have a Cyrix processor so we can try
             * enabling the CPUID instruction and trying again.
             */
            if (firstTime) {
                firstTime = false;
                _CPU_enableCyrixCPUID();
                return CPU_getProcessorType();
                }
            cpu = CPU_i486;
            }
        else
            cpu = CPU_Pentium;
        if (!_CPU_checkClone())
            return cpu | CPU_Intel;
        return cpu;
        }
#elif   defined(__ALPHA__)
    return CPU_Alpha;
#elif   defined(__MIPS__)
    return CPU_Mips;
#elif   defined(__PPC__)
    return CPU_PowerPC;
#endif
}

/****************************************************************************
DESCRIPTION:
Returns true if the processor supports Intel MMX extensions.

HEADER:
cpuinfo.h

RETURNS:
True if MMX is available, false if not.

REMARKS:
This function determines if the processor supports the Intel MMX extended
instruction set.

SEE ALSO:
CPU_getProcessorType, CPU_getProcessorSpeed, CPU_have3DNow, CPU_haveSSE,
CPU_getProcessorName
****************************************************************************/
ibool ZAPI CPU_haveMMX(void)
{
#ifdef  __INTEL__
    if (_CPU_haveCPUID())
        return (_CPU_getCPUIDFeatures() & CPU_HaveMMX) != 0;
    return false;
#else
    return false;
#endif
}

/****************************************************************************
DESCRIPTION:
Returns true if the processor supports AMD 3DNow! extensions.

HEADER:
cpuinfo.h

RETURNS:
True if 3DNow! is available, false if not.

REMARKS:
This function determines if the processor supports the AMD 3DNow! extended
instruction set.

SEE ALSO:
CPU_getProcessorType, CPU_getProcessorSpeed, CPU_haveMMX, CPU_haveSSE,
CPU_getProcessorName
****************************************************************************/
ibool ZAPI CPU_have3DNow(void)
{
#ifdef  __INTEL__
    if (_CPU_haveCPUID())
        return _CPU_have3DNow();
    return false;
#else
    return false;
#endif
}

/****************************************************************************
DESCRIPTION:
Returns true if the processor supports Intel SSE extensions.

HEADER:
cpuinfo.h

RETURNS:
True if Intel SSE is available, false if not.

REMARKS:
This function determines if the processor supports the Intel SSE extended
instruction set.

SEE ALSO:
CPU_getProcessorType, CPU_getProcessorSpeed, CPU_haveMMX, CPU_have3DNow,
CPU_getProcessorName
****************************************************************************/
ibool ZAPI CPU_haveSSE(void)
{
#ifdef  __INTEL__
    if (_CPU_haveCPUID())
        return (_CPU_getCPUIDFeatures() & CPU_HaveSSE) != 0;
    return false;
#else
    return false;
#endif
}

/****************************************************************************
RETURNS:
True if the RTSC instruction is available, false if not.

REMARKS:
This function determines if the processor supports the Intel RDTSC
instruction, for high precision timing. If the processor is not an Intel or
Intel clone CPU, this function will always return false.

DESCRIPTION:
Returns true if the processor supports RDTSC extensions.

HEADER:
cpuinfo.h

RETURNS:
True if RTSC is available, false if not.

REMARKS:
This function determines if the processor supports the RDTSC instruction
for reading the processor time stamp counter.

SEE ALSO:
CPU_getProcessorType, CPU_getProcessorSpeed, CPU_haveMMX, CPU_have3DNow,
CPU_getProcessorName
****************************************************************************/
ibool ZAPI CPU_haveRDTSC(void)
{
#ifdef  __INTEL__
    if (_CPU_haveCPUID())
        return (_CPU_getCPUIDFeatures() & CPU_HaveRDTSC) != 0;
    return false;
#else
    return false;
#endif
}
