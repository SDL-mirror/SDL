/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    QNX Photon GUI SDL driver
    Copyright (C) 2009 Mike Gorchak
    (mike@malva.ua, lestat@i.com.ua)
*/

#include "SDL_config.h"

#include "../SDL_pixels_c.h"
#include "../SDL_yuv_sw_c.h"

#include "SDL_video.h"

#include "SDL_photon_render.h"
#include "SDL_photon.h"

static SDL_Renderer *photon_createrenderer(SDL_Window * window, Uint32 flags);
static int photon_displaymodechanged(SDL_Renderer * renderer);
static int photon_activaterenderer(SDL_Renderer * renderer);
static int photon_createtexture(SDL_Renderer * renderer,
                                SDL_Texture * texture);
static int photon_querytexturepixels(SDL_Renderer * renderer,
                                     SDL_Texture * texture, void **pixels,
                                     int *pitch);
static int photon_settexturepalette(SDL_Renderer * renderer,
                                    SDL_Texture * texture,
                                    const SDL_Color * colors, int firstcolor,
                                    int ncolors);
static int photon_gettexturepalette(SDL_Renderer * renderer,
                                    SDL_Texture * texture, SDL_Color * colors,
                                    int firstcolor, int ncolors);
static int photon_settexturecolormod(SDL_Renderer * renderer,
                                     SDL_Texture * texture);
static int photon_settexturealphamod(SDL_Renderer * renderer,
                                     SDL_Texture * texture);
static int photon_settextureblendmode(SDL_Renderer * renderer,
                                      SDL_Texture * texture);
static int photon_settexturescalemode(SDL_Renderer * renderer,
                                      SDL_Texture * texture);
static int photon_updatetexture(SDL_Renderer * renderer,
                                SDL_Texture * texture, const SDL_Rect * rect,
                                const void *pixels, int pitch);
static int photon_locktexture(SDL_Renderer * renderer, SDL_Texture * texture,
                              const SDL_Rect * rect, int markDirty,
                              void **pixels, int *pitch);
static void photon_unlocktexture(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static void photon_dirtytexture(SDL_Renderer * renderer,
                                SDL_Texture * texture, int numrects,
                                const SDL_Rect * rects);
static int photon_setdrawcolor(SDL_Renderer * renderer);
static int photon_setdrawblendmode(SDL_Renderer * renderer);
static int photon_renderpoint(SDL_Renderer * renderer, int x, int y);
static int photon_renderline(SDL_Renderer * renderer, int x1, int y1, int x2,
                             int y2);
static int photon_renderfill(SDL_Renderer * renderer, const SDL_Rect * rect);
static int photon_rendercopy(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * srcrect,
                             const SDL_Rect * dstrect);
static void photon_renderpresent(SDL_Renderer * renderer);
static void photon_destroytexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static void photon_destroyrenderer(SDL_Renderer * renderer);

static int _photon_recreate_surfaces(SDL_Renderer * renderer);

SDL_RenderDriver photon_renderdriver = {
    photon_createrenderer,
    {
     "photon",
     (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY |
      SDL_RENDERER_PRESENTFLIP2 | SDL_RENDERER_PRESENTFLIP3 |
      SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_PRESENTDISCARD |
      SDL_RENDERER_ACCELERATED),
     (SDL_TEXTUREMODULATE_NONE | SDL_TEXTUREMODULATE_COLOR |
      SDL_TEXTUREMODULATE_ALPHA),
     (SDL_BLENDMODE_NONE | SDL_BLENDMODE_MASK |
      SDL_BLENDMODE_BLEND | SDL_BLENDMODE_ADD | SDL_BLENDMODE_MOD),
     (SDL_TEXTURESCALEMODE_NONE | SDL_TEXTURESCALEMODE_SLOW |
      SDL_TEXTURESCALEMODE_FAST | SDL_TEXTURESCALEMODE_BEST),
     10,
     {SDL_PIXELFORMAT_INDEX8,
      SDL_PIXELFORMAT_RGB555,
      SDL_PIXELFORMAT_RGB565,
      SDL_PIXELFORMAT_RGB24,
      SDL_PIXELFORMAT_RGB888,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_PIXELFORMAT_YV12,
      SDL_PIXELFORMAT_YUY2,
      SDL_PIXELFORMAT_UYVY,
      SDL_PIXELFORMAT_YVYU},
     0,
     0}
};

static SDL_Renderer *
photon_createrenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DisplayData *didata = (SDL_DisplayData *) display->driverdata;
    SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer *renderer = NULL;
    SDL_RenderData *rdata = NULL;

    /* Allocate new renderer structure */
    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(SDL_Renderer));
    if (renderer == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    /* Allocate renderer data */
    rdata = (SDL_RenderData *) SDL_calloc(1, sizeof(SDL_RenderData));
    if (rdata == NULL) {
        SDL_free(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    renderer->DisplayModeChanged = photon_displaymodechanged;
    renderer->ActivateRenderer = photon_activaterenderer;
    renderer->CreateTexture = photon_createtexture;
    renderer->QueryTexturePixels = photon_querytexturepixels;
    renderer->SetTexturePalette = photon_settexturepalette;
    renderer->GetTexturePalette = photon_gettexturepalette;
    renderer->SetTextureAlphaMod = photon_settexturealphamod;
    renderer->SetTextureColorMod = photon_settexturecolormod;
    renderer->SetTextureBlendMode = photon_settextureblendmode;
    renderer->SetTextureScaleMode = photon_settexturescalemode;
    renderer->UpdateTexture = photon_updatetexture;
    renderer->LockTexture = photon_locktexture;
    renderer->UnlockTexture = photon_unlocktexture;
    renderer->DirtyTexture = photon_dirtytexture;
    renderer->SetDrawColor = photon_setdrawcolor;
    renderer->SetDrawBlendMode = photon_setdrawblendmode;
    renderer->RenderPoint = photon_renderpoint;
    renderer->RenderLine = photon_renderline;
    renderer->RenderFill = photon_renderfill;
    renderer->RenderCopy = photon_rendercopy;
    renderer->RenderPresent = photon_renderpresent;
    renderer->DestroyTexture = photon_destroytexture;
    renderer->DestroyRenderer = photon_destroyrenderer;
    renderer->info = photon_renderdriver.info;
    renderer->window = window->id;
    renderer->driverdata = rdata;

    /* Set render acceleration flag in case it is accelerated */
    if ((didata->caps & SDL_PHOTON_ACCELERATED) == SDL_PHOTON_ACCELERATED) {
        renderer->info.flags = SDL_RENDERER_ACCELERATED;
    } else {
        renderer->info.flags &= ~(SDL_RENDERER_ACCELERATED);
    }

    /* Check if upper level requested synchronization on vsync signal */
    if ((flags & SDL_RENDERER_PRESENTVSYNC) == SDL_RENDERER_PRESENTVSYNC) {
        rdata->enable_vsync = SDL_TRUE;
    } else {
        rdata->enable_vsync = SDL_FALSE;
    }

    /* Check what buffer copy/flip scheme is requested */
    rdata->surfaces_count = 0;
    if ((flags & SDL_RENDERER_SINGLEBUFFER) == SDL_RENDERER_SINGLEBUFFER) {
        if ((flags & SDL_RENDERER_PRESENTDISCARD) ==
            SDL_RENDERER_PRESENTDISCARD) {
            renderer->info.flags |=
                SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTDISCARD;
        } else {
            renderer->info.flags |=
                SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY;
        }
        rdata->surfaces_count = 1;
        rdata->surface_visible_idx = 0;
        rdata->surface_render_idx = 0;
    } else {
        if ((flags & SDL_RENDERER_PRESENTFLIP2) == SDL_RENDERER_PRESENTFLIP2) {
            renderer->info.flags |= SDL_RENDERER_PRESENTFLIP2;
            rdata->surfaces_count = 2;
            rdata->surface_visible_idx = 0;
            rdata->surface_render_idx = 1;
        } else {
            if ((flags & SDL_RENDERER_PRESENTFLIP3) ==
                SDL_RENDERER_PRESENTFLIP3) {
                renderer->info.flags |= SDL_RENDERER_PRESENTFLIP3;
                rdata->surfaces_count = 3;
                rdata->surface_visible_idx = 0;
                rdata->surface_render_idx = 1;
            } else {
                renderer->info.flags |=
                    SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY;
                rdata->surfaces_count = 1;
                rdata->surface_visible_idx = 0;
                rdata->surface_render_idx = 0;
            }
        }
    }

    /* Create new graphics context */
    if (rdata->gc==NULL)
    {
       rdata->gc=PgCreateGC(0);
       PgDefaultGC(rdata->gc);
    }

    return renderer;
}

void
photon_addrenderdriver(_THIS)
{
    uint32_t it;

    for (it = 0; it < _this->num_displays; it++) {
        SDL_AddRenderDriver(it, &photon_renderdriver);
    }
}

/****************************************************************************/
/* Render helper functions                                                  */
/****************************************************************************/

static int _photon_recreate_surfaces(SDL_Renderer * renderer)
{
    SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;
    SDL_VideoDisplay *display = NULL;
    SDL_DisplayData *didata = NULL;
    SDL_Window *window = NULL;
    SDL_WindowData *wdata = NULL;
    SDL_VideoData *phdata = NULL;
    uint32_t allocate_task=SDL_PHOTON_SURFTYPE_UNKNOWN;
    int32_t status;

    /* Obtain window and display structures */
    window=SDL_GetWindowFromID(renderer->window);
    wdata=(SDL_WindowData*)window->driverdata;
    display=SDL_GetDisplayFromWindow(window);
    didata=(SDL_DisplayData *) display->driverdata;
    phdata=(SDL_VideoData *) display->device->driverdata;

    /* Check if it is OpenGL ES window */
    if ((window->flags & SDL_WINDOW_OPENGL) == SDL_WINDOW_OPENGL) {
        /* If so, do not waste surfaces */
        rdata->surfaces_type=SDL_PHOTON_SURFTYPE_UNKNOWN;
        return 0;
    }

    if (rdata->surfaces_type==SDL_PHOTON_SURFTYPE_UNKNOWN)
    {
       /* Try to allocate offscreen surfaces */
       allocate_task=SDL_PHOTON_SURFTYPE_OFFSCREEN;
    }
    else
    {
       uint32_t it;

       if (rdata->surfaces_type==SDL_PHOTON_SURFTYPE_OFFSCREEN)
       {
          /* Create offscreen surfaces */
          allocate_task=SDL_PHOTON_SURFTYPE_OFFSCREEN;

          /* Destroy current surfaces */
          for (it=0; it<SDL_PHOTON_MAX_SURFACES; it++)
          {
             if (rdata->osurfaces[it] != NULL)
             {
                PhDCRelease(rdata->osurfaces[it]);
                rdata->osurfaces[it] = NULL;
             }
          }
       }
       else
       {
          if (rdata->surfaces_type==SDL_PHOTON_SURFTYPE_PHIMAGE)
          {
             /* Create shared phimage surfaces */
             allocate_task=SDL_PHOTON_SURFTYPE_PHIMAGE;

             /* Destroy current surfaces */
             for (it=0; it<SDL_PHOTON_MAX_SURFACES; it++)
             {
                if (rdata->pcontexts[it]!=NULL)
                {
                   PmMemReleaseMC(rdata->pcontexts[it]);
                   rdata->pcontexts[it]=NULL;
                }
                if (rdata->psurfaces[it]!=NULL)
                {
                   if (rdata->psurfaces[it]->palette!=NULL)
                   {
                      SDL_free(rdata->psurfaces[it]->palette);
                      rdata->psurfaces[it]->palette=NULL;
                   }
                   /* Destroy shared memory for PhImage_t */
                   PgShmemDestroy(rdata->psurfaces[it]->image);
                   rdata->psurfaces[it]->image=NULL;
                   SDL_free(rdata->psurfaces[it]);
                   rdata->psurfaces[it]=NULL;
                }
             }
          }
       }
    }

    /* Check if current device is not the same as target */
    if (phdata->current_device_id != didata->device_id) {
        /* Set target device as default for Pd and Pg functions */
        status = PdSetTargetDevice(NULL, phdata->rid[didata->device_id]);
        if (status != 0) {
            SDL_SetError("Photon: Can't set default target device\n");
            return -1;
        }
        phdata->current_device_id = didata->device_id;
    }

    switch (allocate_task)
    {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            {
               int32_t it;
               int32_t jt;

               /* Try the hardware accelerated offscreen surfaces first */
               for (it=0; it<rdata->surfaces_count; it++)
               {
                  rdata->osurfaces[it]=PdCreateOffscreenContext(0, window->w, window->h,
                  Pg_OSC_MEM_LINEAR_ACCESSIBLE |
                  /* in case if 2D acceleration is not available use CPU optimized surfaces */
                  Pg_OSC_MEM_HINT_CPU_READ | Pg_OSC_MEM_HINT_CPU_WRITE |
                  /* in case if 2D acceleration is available use it */
                  Pg_OSC_MEM_2D_WRITABLE | Pg_OSC_MEM_2D_READABLE);

                  /* If we can't create an offscreen surface, then fallback to software */
                  if (rdata->osurfaces[it]==NULL)
                  {
                     /* Destroy previously allocated surface(s) */
                     for (jt = it - 1; jt > 0; jt--)
                     {
                        PhDCRelease(rdata->osurfaces[jt]);
                        rdata->osurfaces[jt] = NULL;
                     }
                     break;
                  }
               }

               /* Check if all required surfaces have been created */
               if (rdata->osurfaces[0]!=NULL)
               {
                  rdata->surfaces_type=SDL_PHOTON_SURFTYPE_OFFSCREEN;
                  /* exit from switch if surfaces have been created */
                  break;
               }
               else
               {
                  /* else fall through to software phimage surface allocation */
               }
            }
            /* fall through */
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            {
               int32_t it;
               int32_t jt;
               uint32_t image_pfmt=photon_sdl_to_image_pixelformat(didata->current_mode.format);

               /* Try to allocate the software surfaces in shared memory */
               for (it=0; it<rdata->surfaces_count; it++)
               {
                  /* If this surface with palette, create a palette space */
                  if (image_pfmt==Pg_IMAGE_PALETTE_BYTE)
                  {
                     rdata->psurfaces[it]=PhCreateImage(NULL, window->w, window->h,
                        image_pfmt, NULL, 256, 1);
                  }
                  else
                  {
                     rdata->psurfaces[it]=PhCreateImage(NULL, window->w, window->h,
                        image_pfmt, NULL, 0, 1);
                  }

                  if (rdata->psurfaces[it]!=NULL)
                  {
                     PhPoint_t translation={0, 0};
                     PhDim_t   dimension={window->w, window->h};

                     /* Create memory context for PhImage_t */
                     rdata->pcontexts[it]=PmMemCreateMC(rdata->psurfaces[it], &dimension, &translation);
                  }

                  if ((rdata->psurfaces[it]==NULL) || (rdata->pcontexts[it]==NULL))
                  {
                     /* Destroy previously allocated surface(s) */
                     for (jt = it - 1; jt > 0; jt--)
                     {
                        if (rdata->pcontexts[jt]!=NULL)
                        {
                           PmMemReleaseMC(rdata->pcontexts[it]);
                           rdata->pcontexts[jt]=NULL;
                        }
                        if (rdata->psurfaces[jt]!=NULL)
                        {
                           if (rdata->psurfaces[jt]->palette!=NULL)
                           {
                              SDL_free(rdata->psurfaces[jt]->palette);
                              rdata->psurfaces[jt]->palette=NULL;
                           }
                           /* Destroy shared memory for PhImage_t */
                           PgShmemDestroy(rdata->psurfaces[jt]->image);
                           rdata->psurfaces[jt]->image=NULL;
                           SDL_free(rdata->psurfaces[jt]);
                           rdata->psurfaces[jt] = NULL;
                        }
                     }
                     break;
                  }
               }

               /* Check if all required surfaces have been created */
               if (rdata->psurfaces[0]!=NULL)
               {
                  rdata->surfaces_type=SDL_PHOTON_SURFTYPE_PHIMAGE;
               }
               else
               {
                  rdata->surfaces_type=SDL_PHOTON_SURFTYPE_UNKNOWN;
               }
            }
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            {
               /* do nothing with surface allocation */
               rdata->surfaces_type=SDL_PHOTON_SURFTYPE_UNKNOWN;
            }
            break;
    }

    /* Check if one of two allocation scheme was successful */
    if (rdata->surfaces_type==SDL_PHOTON_SURFTYPE_UNKNOWN)
    {
       SDL_SetError("Photon: primary surface(s) allocation failure");
       return -1;
    }

    /* Store current surface dimensions */
    rdata->window_width=window->w;
    rdata->window_height=window->h;

    /* If current copy/flip scheme is single buffer, then set initial parameters */
    if ((renderer->info.flags & SDL_RENDERER_SINGLEBUFFER)==SDL_RENDERER_SINGLEBUFFER)
    {
       rdata->surface_visible_idx = 0;
       rdata->surface_render_idx = 0;
    }

    /* If current copy/flip scheme is double buffer, then set initial parameters */
    if ((renderer->info.flags & SDL_RENDERER_PRESENTFLIP2)==SDL_RENDERER_PRESENTFLIP2)
    {
       rdata->surface_visible_idx = 0;
       rdata->surface_render_idx = 1;
    }

    /* If current copy/flip scheme is triple buffer, then set initial parameters */
    if ((renderer->info.flags & SDL_RENDERER_PRESENTFLIP3)==SDL_RENDERER_PRESENTFLIP3)
    {
       rdata->surface_visible_idx = 0;
       rdata->surface_render_idx = 1;
    }

    switch (rdata->surfaces_type)
    {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            PgSetGCCx(rdata->osurfaces[rdata->surface_render_idx], rdata->gc);
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PgSetGCCx(rdata->pcontexts[rdata->surface_render_idx], rdata->gc);
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            break;
    }

    return 0;
}

int _photon_update_rectangles(SDL_Renderer* renderer, PhRect_t* rect)
{
   SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;
   SDL_Window *window = window=SDL_GetWindowFromID(renderer->window);
   SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
   PhPoint_t src_point;

   /* If currently single buffer is in use, we have to flush all data */
   if (rdata->surface_render_idx==rdata->surface_visible_idx)
   {
      /* Flush all undrawn graphics data to surface */
      switch (rdata->surfaces_type)
      {
          case SDL_PHOTON_SURFTYPE_OFFSCREEN:
               PgFlushCx(rdata->osurfaces[rdata->surface_visible_idx]);
               PgWaitHWIdle();
               break;
          case SDL_PHOTON_SURFTYPE_PHIMAGE:
               PmMemFlush(rdata->pcontexts[rdata->surface_visible_idx], rdata->psurfaces[rdata->surface_visible_idx]);
               break;
          case SDL_PHOTON_SURFTYPE_UNKNOWN:
               return;
      }
   }

   PgSetRegionCx(PhDCGetCurrent(), PtWidgetRid(wdata->window));

   src_point.x = rect->ul.x;
   src_point.y = rect->ul.y;

   switch (rdata->surfaces_type)
   {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            PgContextBlit(rdata->osurfaces[rdata->surface_visible_idx], rect, NULL, rect);
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PgDrawPhImageRectv(&src_point, rdata->psurfaces[rdata->surface_visible_idx], rect, 0);
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            break;
   }
}

/****************************************************************************/
/* SDL render interface                                                     */
/****************************************************************************/

static int
photon_activaterenderer(SDL_Renderer * renderer)
{
   SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;
   SDL_Window *window = SDL_GetWindowFromID(renderer->window);
   SDL_WindowData *wdata = (SDL_WindowData *)window->driverdata;

   if ((rdata->window_width!=window->w) || (rdata->window_height!=window->h))
   {
      return _photon_recreate_surfaces(renderer);
   }

   switch (rdata->surfaces_type)
   {
      case SDL_PHOTON_SURFTYPE_OFFSCREEN:
           PgSetGCCx(rdata->osurfaces[rdata->surface_render_idx], rdata->gc);
           break;
      case SDL_PHOTON_SURFTYPE_PHIMAGE:
           PgSetGCCx(rdata->pcontexts[rdata->surface_render_idx], rdata->gc);
           break;
      case SDL_PHOTON_SURFTYPE_UNKNOWN:
           break;
   }

   return 0;
}

static int
photon_displaymodechanged(SDL_Renderer * renderer)
{
    return _photon_recreate_surfaces(renderer);
}

static int
photon_createtexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_RenderData *renderdata = (SDL_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_TextureData *tdata = NULL;

    /* Allocate texture driver data */
    tdata = (SDL_TextureData *) SDL_calloc(1, sizeof(SDL_TextureData));
    if (tdata == NULL) {
        SDL_OutOfMemory();
        return -1;
    }

    /* Set texture driver data */
    texture->driverdata = tdata;
}

static int
photon_querytexturepixels(SDL_Renderer * renderer, SDL_Texture * texture,
                          void **pixels, int *pitch)
{
}

static int
photon_settexturepalette(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Color * colors, int firstcolor,
                         int ncolors)
{
}

static int
photon_gettexturepalette(SDL_Renderer * renderer, SDL_Texture * texture,
                         SDL_Color * colors, int firstcolor, int ncolors)
{
}

static int
photon_settexturecolormod(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static int
photon_settexturealphamod(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static int
photon_settextureblendmode(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static int
photon_settexturescalemode(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static int
photon_updatetexture(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Rect * rect, const void *pixels, int pitch)
{
}

static int
photon_locktexture(SDL_Renderer * renderer, SDL_Texture * texture,
                   const SDL_Rect * rect, int markDirty, void **pixels,
                   int *pitch)
{
}

static void
photon_unlocktexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static void
photon_dirtytexture(SDL_Renderer * renderer, SDL_Texture * texture,
                    int numrects, const SDL_Rect * rects)
{
}

static int
photon_setdrawcolor(SDL_Renderer * renderer)
{
   SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;

   switch (rdata->surfaces_type)
   {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PgSetFillColorCx(rdata->gc, PgRGB(renderer->r, renderer->g, renderer->b));
            PgSetStrokeColorCx(rdata->gc, PgRGB(renderer->r, renderer->g, renderer->b));
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            break;
   }
}

static int
photon_setdrawblendmode(SDL_Renderer * renderer)
{
}

static int
photon_renderpoint(SDL_Renderer * renderer, int x, int y)
{
   SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;

   switch (rdata->surfaces_type)
   {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            PgDrawIPixelCx(rdata->osurfaces[rdata->surface_render_idx], x, y);
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PgDrawIPixelCx(rdata->pcontexts[rdata->surface_render_idx], x, y);
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            break;
   }
}

static int
photon_renderline(SDL_Renderer * renderer, int x1, int y1, int x2, int y2)
{
   SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;

   switch (rdata->surfaces_type)
   {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            PgDrawILineCx(rdata->osurfaces[rdata->surface_render_idx], x1, y1, x2, y2);
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PgDrawILineCx(rdata->pcontexts[rdata->surface_render_idx], x1, y1, x2, y2);
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            break;
   }
}

static int
photon_renderfill(SDL_Renderer * renderer, const SDL_Rect * rect)
{
   SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;

   switch (rdata->surfaces_type)
   {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            PgDrawIRectCx(rdata->osurfaces[rdata->surface_render_idx], rect->x, rect->y, rect->w+rect->x-1, rect->h+rect->y-1, Pg_DRAW_FILL);
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PgDrawIRectCx(rdata->pcontexts[rdata->surface_render_idx], rect->x, rect->y, rect->w+rect->x-1, rect->h+rect->y-1, Pg_DRAW_FILL);
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            break;
   }
}

static int
photon_rendercopy(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
}

static void
photon_renderpresent(SDL_Renderer * renderer)
{
   SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;
   SDL_Window *window = window=SDL_GetWindowFromID(renderer->window);
   SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
   PhRect_t src_rect;
   PhPoint_t src_point;

   /* Flush all undrawn graphics data to surface */
   switch (rdata->surfaces_type)
   {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            PgFlushCx(rdata->osurfaces[rdata->surface_render_idx]);
            PgWaitHWIdle();
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PmMemFlush(rdata->pcontexts[rdata->surface_render_idx], rdata->psurfaces[rdata->surface_render_idx]);
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            return;
   }

   PgFFlush(Ph_START_DRAW);
   PgSetRegionCx(PhDCGetCurrent(), PtWidgetRid(wdata->window));

   /* Set blit area */
   src_rect.ul.x = 0;
   src_rect.ul.y = 0;
   src_rect.lr.x = rdata->window_width - 1;
   src_rect.lr.y = rdata->window_height - 1;

   src_point.x = 0;
   src_point.y = 0;

   switch (rdata->surfaces_type)
   {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            PgContextBlit(rdata->osurfaces[rdata->surface_render_idx], &src_rect, NULL, &src_rect);
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            PgDrawPhImagev(&src_point, rdata->psurfaces[rdata->surface_render_idx], 0);
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            break;
   }

   /* finish blit */
   PgFFlush(Ph_DONE_DRAW);
   PgWaitHWIdle();
}

static void
photon_destroytexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static void
photon_destroyrenderer(SDL_Renderer * renderer)
{
    SDL_RenderData *rdata = (SDL_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_WindowData *wdata = (SDL_WindowData *)window->driverdata;
    uint32_t it;

    /* Destroy graphics context */
    if (rdata->gc!=NULL)
    {
       PgDestroyGC(rdata->gc);
       rdata->gc=NULL;
    }

    switch (rdata->surfaces_type)
    {
       case SDL_PHOTON_SURFTYPE_OFFSCREEN:
            {
               /* Destroy current surfaces */
               for (it=0; it<SDL_PHOTON_MAX_SURFACES; it++)
               {
                  if (rdata->osurfaces[it] != NULL)
                  {
                     PhDCRelease(rdata->osurfaces[it]);
                     rdata->osurfaces[it] = NULL;
                  }
               }
            }
            break;
       case SDL_PHOTON_SURFTYPE_PHIMAGE:
            {
               /* Destroy current surfaces */
               for (it=0; it<SDL_PHOTON_MAX_SURFACES; it++)
               {
                  if (rdata->pcontexts[it]!=NULL)
                  {
                     PmMemReleaseMC(rdata->pcontexts[it]);
                     rdata->pcontexts[it]=NULL;
                  }
                  if (rdata->psurfaces[it]!=NULL)
                  {
                     if (rdata->psurfaces[it]->palette!=NULL)
                     {
                        SDL_free(rdata->psurfaces[it]->palette);
                        rdata->psurfaces[it]->palette=NULL;
                     }
                     /* Destroy shared memory for PhImage_t */
                     PgShmemDestroy(rdata->psurfaces[it]->image);
                     rdata->psurfaces[it]->image=NULL;
                     SDL_free(rdata->psurfaces[it]);
                     rdata->psurfaces[it]=NULL;
                  }
               }
            }
            break;
       case SDL_PHOTON_SURFTYPE_UNKNOWN:
            {
               /* nothing to do */
            }
            break;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
