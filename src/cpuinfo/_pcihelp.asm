;****************************************************************************
;*
;*                  SciTech OS Portability Manager Library
;*
;*  ========================================================================
;*
;*   Copyright (C) 1991-2002 SciTech Software, Inc. All rights reserved.
;*
;*   This file may be distributed and/or modified under the terms of the
;*   GNU Lesser General Public License version 2.1 as published by the Free
;*   Software Foundation and appearing in the file LICENSE.LGPL included
;*   in the packaging of this file.
;*
;*   Licensees holding a valid Commercial License for this product from
;*   SciTech Software, Inc. may use this file in accordance with the
;*   Commercial License Agreement provided with the Software.
;*
;*   This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
;*   THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
;*   PURPOSE.
;*
;*   See http://www.scitechsoft.com/license/ for information about
;*   the licensing options available and how to purchase a Commercial
;*   License Agreement.
;*
;*   Contact license@scitechsoft.com if any conditions of this licensing
;*   are not clear to you, or you have questions about licensing options.
;*
;*  ========================================================================
;*
;* Language:    NASM
;* Environment: Any
;*
;* Description: Helper assembler functions for PCI access module.
;*
;****************************************************************************

include "scitech.mac"           ; Memory model macros

header  _pcilib

begcodeseg  _pcilib

ifdef flatmodel

;----------------------------------------------------------------------------
; uchar _ASMAPI _BIOS32_service(
;   ulong service,
;   ulong func,
;   ulong *physBase,
;   ulong *length,
;   ulong *serviceOffset,
;   PCIBIOS_entry entry);
;----------------------------------------------------------------------------
; Call the BIOS32 services directory
;----------------------------------------------------------------------------
cprocstart   _BIOS32_service

        ARG     service:ULONG, func:ULONG, physBase:DPTR, len:DPTR, off:DPTR, entry:QWORD

        enter_c
        mov     eax,[service]
        mov     ebx,[func]
        call    far dword [entry]
        mov     esi,[physBase]
        mov     [esi],ebx
        mov     esi,[len]
        mov     [esi],ecx
        mov     esi,[off]
        mov     [esi],edx
        leave_c
        ret

cprocend

endif

;----------------------------------------------------------------------------
; ushort _ASMAPI _PCIBIOS_isPresent(ulong i_eax,ulong *o_edx,ushort *oeax,
;   uchar *o_cl,PCIBIOS_entry entry)
;----------------------------------------------------------------------------
; Call the PCI BIOS to determine if it is present.
;----------------------------------------------------------------------------
cprocstart   _PCIBIOS_isPresent

        ARG     i_eax:ULONG, o_edx:DPTR, oeax:DPTR, o_cl:DPTR, entry:QWORD

        enter_c
        mov     eax,[i_eax]
ifdef   flatmodel
        call    far dword [entry]
else
        int     1Ah
endif
        _les    _si,[o_edx]
        mov     [_ES _si],edx
        _les    _si,[oeax]
        mov     [_ES _si],ax
        _les    _si,[o_cl]
        mov     [_ES _si],cl
        mov     ax,bx
        leave_c
        ret

cprocend

;----------------------------------------------------------------------------
; ulong _PCIBIOS_service(ulong r_eax,ulong r_ebx,ulong r_edi,ulong r_ecx,
;   PCIBIOS_entry entry)
;----------------------------------------------------------------------------
; Call the PCI BIOS services, either via the 32-bit protected mode entry
; point or via the Int 1Ah 16-bit interrupt.
;----------------------------------------------------------------------------
cprocstart   _PCIBIOS_service

        ARG     r_eax:ULONG, r_ebx:ULONG, r_edi:ULONG, r_ecx:ULONG, entry:QWORD

        enter_c
        mov     eax,[r_eax]
        mov     ebx,[r_ebx]
        mov     edi,[r_edi]
        mov     ecx,[r_ecx]
ifdef   flatmodel
        call    far dword [entry]
else
        int     1Ah
endif
        mov     eax,ecx
ifndef  flatmodel
        shld    edx,eax,16      ; Return result in DX:AX
endif
        leave_c
        ret

cprocend

;----------------------------------------------------------------------------
; int _PCIBIOS_getRouting(PCIRoutingOptionsBuffer *buf,PCIBIOS_entry entry);
;----------------------------------------------------------------------------
; Get the routing options for PCI devices
;----------------------------------------------------------------------------
cprocstart   _PCIBIOS_getRouting

        ARG     buf:DPTR, entry:QWORD

        enter_c
        mov     eax,0B10Eh
        mov     bx,0
        _les    _di,[buf]
ifdef   flatmodel
        call    far dword [entry]
else
        int     1Ah
endif
        movzx   eax,ah
        leave_c
        ret

cprocend

;----------------------------------------------------------------------------
; ibool _PCIBIOS_setIRQ(int busDev,int intPin,int IRQ,PCIBIOS_entry entry);
;----------------------------------------------------------------------------
; Change the IRQ routing for the PCI device
;----------------------------------------------------------------------------
cprocstart   _PCIBIOS_setIRQ

        ARG     busDev:UINT, intPin:UINT, IRQ:UINT, entry:QWORD

        enter_c
        mov     eax,0B10Fh
        mov     bx,[USHORT busDev]
        mov     cl,[BYTE intPin]
        mov     ch,[BYTE IRQ]
ifdef   flatmodel
        call    far dword [entry]
else
        int     1Ah
endif
        mov     eax,1
        jnc     @@1
        xor     eax,eax         ; Function failed!
@@1:    leave_c
        ret

cprocend

;----------------------------------------------------------------------------
; ulong _PCIBIOS_specialCycle(int bus,ulong data,PCIBIOS_entry entry);
;----------------------------------------------------------------------------
; Generate a special cycle via the PCI BIOS.
;----------------------------------------------------------------------------
cprocstart   _PCIBIOS_specialCycle

        ARG     bus:UINT, data:ULONG, entry:QWORD

        enter_c
        mov     eax,0B106h
        mov     bh,[BYTE bus]
        mov     ecx,[data]
ifdef   flatmodel
        call    far dword [entry]
else
        int     1Ah
endif
        leave_c
        ret

cprocend

;----------------------------------------------------------------------------
; ushort _PCI_getCS(void)
;----------------------------------------------------------------------------
cprocstart   _PCI_getCS

        mov     ax,cs
        ret

cprocend

;----------------------------------------------------------------------------
; int PM_inpb(int port)
;----------------------------------------------------------------------------
; Reads a byte from the specified port
;----------------------------------------------------------------------------
cprocstart  PM_inpb

        ARG     port:UINT

        push    _bp
        mov     _bp,_sp
        xor     _ax,_ax
        mov     _dx,[port]
        in      al,dx
        pop     _bp
        ret

cprocend

;----------------------------------------------------------------------------
; int PM_inpw(int port)
;----------------------------------------------------------------------------
; Reads a word from the specified port
;----------------------------------------------------------------------------
cprocstart  PM_inpw

        ARG     port:UINT

        push    _bp
        mov     _bp,_sp
        xor     _ax,_ax
        mov     _dx,[port]
        in      ax,dx
        pop     _bp
        ret

cprocend

;----------------------------------------------------------------------------
; ulong PM_inpd(int port)
;----------------------------------------------------------------------------
; Reads a word from the specified port
;----------------------------------------------------------------------------
cprocstart  PM_inpd

        ARG     port:UINT

        push    _bp
        mov     _bp,_sp
        mov     _dx,[port]
        in      eax,dx
ifndef flatmodel
        shld    edx,eax,16      ; DX:AX = result
endif
        pop     _bp
        ret

cprocend

;----------------------------------------------------------------------------
; void PM_outpb(int port,int value)
;----------------------------------------------------------------------------
; Write a byte to the specified port.
;----------------------------------------------------------------------------
cprocstart  PM_outpb

        ARG     port:UINT, value:UINT

        push    _bp
        mov     _bp,_sp
        mov     _dx,[port]
        mov     _ax,[value]
        out     dx,al
        pop     _bp
        ret

cprocend

;----------------------------------------------------------------------------
; void PM_outpw(int port,int value)
;----------------------------------------------------------------------------
; Write a word to the specified port.
;----------------------------------------------------------------------------
cprocstart  PM_outpw

        ARG     port:UINT, value:UINT

        push    _bp
        mov     _bp,_sp
        mov     _dx,[port]
        mov     _ax,[value]
        out     dx,ax
        pop     _bp
        ret

cprocend

;----------------------------------------------------------------------------
; void PM_outpd(int port,ulong value)
;----------------------------------------------------------------------------
; Write a word to the specified port.
;----------------------------------------------------------------------------
cprocstart  PM_outpd

        ARG     port:UINT, value:ULONG

        push    _bp
        mov     _bp,_sp
        mov     _dx,[port]
        mov     eax,[value]
        out     dx,eax
        pop     _bp
        ret

cprocend

endcodeseg  _pcilib

        END
