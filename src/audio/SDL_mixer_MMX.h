// headers for MMX assembler version of SDL_MixAudio
// Copyright 2002 Stephane Marchesin (stephane.marchesin@wanadoo.fr)
// This code is licensed under the LGPL (see COPYING for details)
// 
// Assumes buffer size in bytes is a multiple of 16
// Assumes SDL_MIX_MAXVOLUME = 128


#if defined(i386) && defined(__GNUC__) && defined(USE_ASMBLIT)
void SDL_MixAudio_MMX_S16(char* ,char* ,unsigned int ,int );
void SDL_MixAudio_MMX_S8(char* ,char* ,unsigned int ,int );
#endif

