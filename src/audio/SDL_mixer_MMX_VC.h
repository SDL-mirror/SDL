#if defined(USE_ASM_MIXER_VC)
// headers for MMX assembler version of SDL_MixAudio
// Copyright 2002 Stephane Marchesin (stephane.marchesin@wanadoo.fr)
// Converted to Intel ASM notation by Cth
// This code is licensed under the LGPL (see COPYING for details)
// 
// Assumes buffer size in bytes is a multiple of 16
// Assumes SDL_MIX_MAXVOLUME = 128

void SDL_MixAudio_MMX_S16_VC(char* ,char* ,unsigned int ,int );
void SDL_MixAudio_MMX_S8_VC(char* ,char* ,unsigned int ,int );
#endif
