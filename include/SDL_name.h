
#ifndef _SDLname_h_
#define _SDLname_h_

#if defined(__STDC__) || defined(__cplusplus)
#define NeedFunctionPrototypes 1
#endif

#ifndef SDL_NAME
#define SDL_NAME(X)	SDL_##X
#endif

#endif /* _SDLname_h_ */
