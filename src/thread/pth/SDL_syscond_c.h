#ifndef _SDL_SYSCOND_C_H_
#define _SDL_SYSCOND_C_H_

struct SDL_cond
{
	pth_cond_t	condpth_p;
};

#endif /* _SDL_SYSCOND_C_H_ */
