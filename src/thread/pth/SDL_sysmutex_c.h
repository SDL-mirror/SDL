#ifndef _SDL_SYSMUTEX_C_H_
#define _SDL_SYSMUTEX_C_H_

struct SDL_mutex {
	pth_mutex_t mutexpth_p;
};

#endif /* _SDL_SYSMUTEX_C_H_ */
