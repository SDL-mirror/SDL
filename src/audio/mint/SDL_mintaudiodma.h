/*
 * STE/TT 8 bits audio dma registers
 * 
 * Patrice Mandin
 */

#ifndef _SDL_mintdma_h
#define _SDL_mintdma_h

#define DMAAUDIO_IO_BASE (0xffff8900)
struct DMAAUDIO_IO_S {
	unsigned char int_ctrl;
	unsigned char control;

	unsigned char dummy1;
	unsigned char start_high;
	unsigned char dummy2;
	unsigned char start_mid;
	unsigned char dummy3;
	unsigned char start_low;

	unsigned char dummy4;
	unsigned char cur_high;
	unsigned char dummy5;
	unsigned char cur_mid;
	unsigned char dummy6;
	unsigned char cur_low;

	unsigned char dummy7;
	unsigned char end_high;
	unsigned char dummy8;
	unsigned char end_mid;
	unsigned char dummy9;
	unsigned char end_low;

	unsigned char dummy10[12];

	unsigned char track_select; /* CODEC only */
	unsigned char mode;
};
#define DMAAUDIO_IO ((*(volatile struct DMAAUDIO_IO_S *)DMAAUDIO_IO_BASE))

#endif /* _SDL_mintdma_h */
