// MMX assembler version of SDL_MixAudio for signed little endian 16 bit samples and signed 8 bit samples
// Copyright 2002 Stephane Marchesin (stephane.marchesin@wanadoo.fr)
// This code is licensed under the LGPL (see COPYING for details)
// 
// Assumes buffer size in bytes is a multiple of 16
// Assumes SDL_MIX_MAXVOLUME = 128


////////////////////////////////////////////////
// Mixing for 16 bit signed buffers
////////////////////////////////////////////////

#if defined(i386) && defined(__GNUC__) && defined(USE_ASMBLIT)
void SDL_MixAudio_MMX_S16(char* dst,char* src,unsigned int size,int volume)
{
    __asm__ __volatile__ (

"	movl %0,%%edi\n"	// edi = dst
"	movl %1,%%esi\n"	// esi = src
"	movl %3,%%eax\n"	// eax = volume

"	movl %2,%%ebx\n"	// ebx = size

"	shrl $4,%%ebx\n"	// process 16 bytes per iteration = 8 samples

"	jz .endS16\n"

"	pxor %%mm0,%%mm0\n"

"	movd %%eax,%%mm0\n"
"	movq %%mm0,%%mm1\n"
"	psllq $16,%%mm0\n"
"	por %%mm1,%%mm0\n"
"	psllq $16,%%mm0\n"
"	por %%mm1,%%mm0\n"
"	psllq $16,%%mm0\n"
"	por %%mm1,%%mm0\n"		// mm0 = vol|vol|vol|vol

".align 16\n"
"	.mixloopS16:\n"

"	movq (%%esi),%%mm1\n" // mm1 = a|b|c|d

"	movq %%mm1,%%mm2\n" // mm2 = a|b|c|d

"	movq 8(%%esi),%%mm4\n" // mm4 = e|f|g|h

	// pré charger le buffer dst dans mm7
"	movq (%%edi),%%mm7\n" // mm7 = dst[0]"

	// multiplier par le volume
"	pmullw %%mm0,%%mm1\n" // mm1 = l(a*v)|l(b*v)|l(c*v)|l(d*v)

"	pmulhw %%mm0,%%mm2\n" // mm2 = h(a*v)|h(b*v)|h(c*v)|h(d*v)
"	movq %%mm4,%%mm5\n" // mm5 = e|f|g|h

"	pmullw %%mm0,%%mm4\n" // mm4 = l(e*v)|l(f*v)|l(g*v)|l(h*v)

"	pmulhw %%mm0,%%mm5\n" // mm5 = h(e*v)|h(f*v)|h(g*v)|h(h*v)
"	movq %%mm1,%%mm3\n" // mm3 = l(a*v)|l(b*v)|l(c*v)|l(d*v)

"	punpckhwd %%mm2,%%mm1\n" // mm1 = a*v|b*v

"	movq %%mm4,%%mm6\n" // mm6 = l(e*v)|l(f*v)|l(g*v)|l(h*v)
"	punpcklwd %%mm2,%%mm3\n" // mm3 = c*v|d*v

"	punpckhwd %%mm5,%%mm4\n" // mm4 = e*f|f*v

"	punpcklwd %%mm5,%%mm6\n" // mm6 = g*v|h*v

	// pré charger le buffer dst dans mm5
"	movq 8(%%edi),%%mm5\n" // mm5 = dst[1]

	// diviser par 128
"	psrad $7,%%mm1\n" // mm1 = a*v/128|b*v/128 , 128 = SDL_MIX_MAXVOLUME
"	addl $16,%%esi\n"

"	psrad $7,%%mm3\n" // mm3 = c*v/128|d*v/128

"	psrad $7,%%mm4\n" // mm4 = e*v/128|f*v/128

	// mm1 = le sample avec le volume modifié
"	packssdw %%mm1,%%mm3\n" // mm3 = s(a*v|b*v|c*v|d*v)

"	psrad $7,%%mm6\n" // mm6= g*v/128|h*v/128
"	paddsw %%mm7,%%mm3\n" // mm3 = adjust_volume(src)+dst

	// mm4 = le sample avec le volume modifié
"	packssdw %%mm4,%%mm6\n" // mm6 = s(e*v|f*v|g*v|h*v)
"	movq %%mm3,(%%edi)\n"

"	paddsw %%mm5,%%mm6\n" // mm6 = adjust_volume(src)+dst

"	movq %%mm6,8(%%edi)\n"

"	addl $16,%%edi\n"

"	dec %%ebx\n"

"	jnz .mixloopS16\n"

"	emms\n"

".endS16:\n"
	 :
	 : "m" (dst), "m"(src),"m"(size),
	 "m"(volume)
	 : "eax","ebx", "esi", "edi","memory"
	 );
}



////////////////////////////////////////////////
// Mixing for 8 bit signed buffers
////////////////////////////////////////////////

void SDL_MixAudio_MMX_S8(char* dst,char* src,unsigned int size,int volume)
{
    __asm__ __volatile__ (

"	movl %0,%%edi\n"	// edi = dst
"	movl %1,%%esi\n"	// esi = src
"	movl %3,%%eax\n"	// eax = volume

"	movd %%ebx,%%mm0\n"
"	movq %%mm0,%%mm1\n"
"	psllq $16,%%mm0\n"
"	por %%mm1,%%mm0\n"
"	psllq $16,%%mm0\n"
"	por %%mm1,%%mm0\n"
"	psllq $16,%%mm0\n"
"	por %%mm1,%%mm0\n"

"	movl %2,%%ebx\n"	// ebx = size
"	shr $3,%%ebx\n"	// process 8 bytes per iteration = 8 samples

"	cmp $0,%%ebx\n"
"	je .endS8\n"

".align 16\n"
"	.mixloopS8:\n"

"	pxor %%mm2,%%mm2\n"		// mm2 = 0
"	movq (%%esi),%%mm1\n"	// mm1 = a|b|c|d|e|f|g|h

"	movq %%mm1,%%mm3\n" 	// mm3 = a|b|c|d|e|f|g|h

	// on va faire le "sign extension" en faisant un cmp avec 0 qui retourne 1 si <0, 0 si >0
"	pcmpgtb %%mm1,%%mm2\n"	// mm2 = 11111111|00000000|00000000....

"	punpckhbw %%mm2,%%mm1\n"	// mm1 = 0|a|0|b|0|c|0|d

"	punpcklbw %%mm2,%%mm3\n"	// mm3 = 0|e|0|f|0|g|0|h
"	movq (%%edi),%%mm2\n"	// mm2 = destination

"	pmullw %%mm0,%%mm1\n"	// mm1 = v*a|v*b|v*c|v*d
"	addl $8,%%esi\n"

"	pmullw %%mm0,%%mm3\n"	// mm3 = v*e|v*f|v*g|v*h
"	psraw $7,%%mm1\n"		// mm1 = v*a/128|v*b/128|v*c/128|v*d/128 

"	psraw $7,%%mm3\n"		// mm3 = v*e/128|v*f/128|v*g/128|v*h/128

"	packsswb %%mm1,%%mm3\n"	// mm1 = v*a/128|v*b/128|v*c/128|v*d/128|v*e/128|v*f/128|v*g/128|v*h/128

"	paddsb %%mm2,%%mm3\n"	// add to destination buffer

"	movq %%mm3,(%%edi)\n"	// store back to ram
"	addl $8,%%edi\n"

"	dec %%ebx\n"

"	jnz .mixloopS8\n"

".endS8:\n"
"	emms\n"
	 :
	 : "m" (dst), "m"(src),"m"(size),
	 "m"(volume)
	 : "eax","ebx", "esi", "edi","memory"
	 );
}
#endif

