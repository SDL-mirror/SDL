/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_RENDER_AMIGAOS4 && !SDL_RENDER_DISABLED

#include "SDL_render_compositing.h"

#include <proto/graphics.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static Uint32 OS4_GetPixel(Uint32 * memory, Uint32 width, uint16 x, uint16 y)
{
	return *(memory + x + y * width);
}

static void OS4_SetPixel(Uint32 * memory, Uint32 width, uint16 x, uint16 y, Uint32 color)
{
	*(memory + x + y * width) = color;
}

static Uint8 ALPHA(Uint32 c)
{
	return (c >> 24);
}

static Uint8 RED(Uint32 c)
{
	return (c >> 16);
}

static Uint8 GREEN(Uint32 c)
{
	return (c >> 8);
}

static Uint8 BLUE(Uint32 c)
{
	return c & 255;
}

static Uint8 MUL(Uint8 a, Uint8 b)
{
	Uint32 u = (a * b) / 255;
	return u;
}

static Uint8 ADD(Uint8 a, Uint8 b)
{
	Uint32 c = a + b;

	if (c > 255) c = 255;

	return c;
}

static Uint32 BlendPoint(SDL_Renderer * renderer, Uint32 old)
{
	Uint8 sr, sg, sb, sa, dr, dg, db, da;

	sr = MUL(renderer->a, renderer->r);
	sg = MUL(renderer->a, renderer->g);
	sb = MUL(renderer->a, renderer->b);
	sa = renderer->a;

	dr = sr + MUL((255 - renderer->a), RED(old));
	dg = sg + MUL((255 - renderer->a), GREEN(old));
	db = sb + MUL((255 - renderer->a), BLUE(old));
	da = sa + MUL((255 - renderer->a), ALPHA(old));

	return da << 24 | dr << 16 | dg << 8 | db;
}

static Uint32 AddPoint(SDL_Renderer * renderer, Uint32 old)
{
	Uint8 sr, sg, sb, dr, dg, db, da;

	sr = MUL(renderer->a, renderer->r);
	sg = MUL(renderer->a, renderer->g);
	sb = MUL(renderer->a, renderer->b);

    dr = ADD(sr, RED(old));
	dg = ADD(sg, GREEN(old));
	db = ADD(sb, BLUE(old));
	da = ALPHA(old);

	return da << 24 | dr << 16 | dg << 8 | db;
}

static Uint32 ModPoint(SDL_Renderer * renderer, Uint32 old)
{
	Uint8 dr, dg, db, da;

	dr = MUL(renderer->r, RED(old));
	dg = MUL(renderer->g, GREEN(old));
	db = MUL(renderer->b, BLUE(old));
	da = ALPHA(old);

	return da << 24 | dr << 16 | dg << 8 | db;
}

static Uint32 NopPoint(SDL_Renderer * renderer, Uint32 old)
{
	return old;
}

int
OS4_RenderDrawPoints(SDL_Renderer * renderer, const SDL_FPoint * points,
                    int count)
{
    OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
    struct BitMap *bitmap = OS4_ActivateRenderer(renderer);
    SDL_Point *final_points;
    int i, status;

    if (!bitmap) {
        return -1;
    }

    final_points = SDL_stack_alloc(SDL_Point, count);
    if (!final_points) {
        return SDL_OutOfMemory();
    }
    if (renderer->viewport.x || renderer->viewport.y) {
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)(x + points[i].x);
            final_points[i].y = (int)(y + points[i].y);
        }
    } else {
        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)points[i].x;
            final_points[i].y = (int)points[i].y;
        }
    }

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {

        int32 ret = 0;

        Uint32 color =
            renderer->a << 24 |
            renderer->r << 16 |
            renderer->g << 8 |
            renderer->b;

        // TODO: clipping?
        for (i = 0; i < count; ++i) {
            ret |= data->iGraphics->WritePixelColor(
                &data->rastport,
                final_points[i].x,
                final_points[i].y,
                color);
        }

        status = ret ? -1 : 0;

    } else {

		APTR baseaddress;
		uint32 bytesperrow;

		APTR lock = data->iGraphics->LockBitMapTags(
			bitmap,
    		LBM_BaseAddress, &baseaddress,
	 		LBM_BytesPerRow, &bytesperrow,
			TAG_DONE);

		if (lock) {

			Uint32 (*blendfp)(SDL_Renderer *, Uint32);

			Uint32 width = bytesperrow / 4;

			switch (renderer->blendMode) {
				case SDL_BLENDMODE_BLEND:
					blendfp = BlendPoint;
					break;
				case SDL_BLENDMODE_ADD:
					blendfp = AddPoint;
					break;
				case SDL_BLENDMODE_MOD:
					blendfp = ModPoint;
					break;
				default:
					dprintf("Unknown blend mode %d\n", renderer->blendMode);
					blendfp = NopPoint;
					break;
			}

			for (i = 0; i < count; ++i) {
				Uint32 newcolor, oldcolor;

				oldcolor = OS4_GetPixel(baseaddress, width, final_points[i].x, final_points[i].y);

				newcolor = blendfp(renderer, oldcolor);

				OS4_SetPixel(baseaddress, width, final_points[i].x, final_points[i].y, newcolor);
			}

			data->iGraphics->UnlockBitMap(lock);

			status = 0;
		} else {
			dprintf("Lock failed\n");
    		status = -1;
		}
    }

    SDL_stack_free(final_points);

    return status;
}

int
OS4_RenderDrawLines(SDL_Renderer * renderer, const SDL_FPoint * points,
                   int count)
{
    OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
    struct BitMap *bitmap = OS4_ActivateRenderer(renderer);

    SDL_Point *final_points;
    int i, status;

    if (!bitmap) {
        return -1;
    }

    final_points = SDL_stack_alloc(SDL_Point, count);
    if (!final_points) {
        return SDL_OutOfMemory();
    }
    if (renderer->viewport.x || renderer->viewport.y) {
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)(x + points[i].x);
            final_points[i].y = (int)(y + points[i].y);
        }
    } else {
        for (i = 0; i < count; ++i) {
            final_points[i].x = (int)points[i].x;
            final_points[i].y = (int)points[i].y;
        }
    }

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {

        Uint32 color =
            renderer->a << 24 |
            renderer->r << 16 |
            renderer->g << 8 |
            renderer->b;

        data->iGraphics->SetRPAttrs(&data->rastport, RPTAG_APenColor, color, TAG_DONE);

        // TODO: clipping?
        for (i = 0; i < count - 1; ++i) {
            data->iGraphics->Move(
                &data->rastport,
                final_points[i].x,
                final_points[i].y);

            data->iGraphics->Draw(
                &data->rastport,
                final_points[i + 1].x,
                final_points[i + 1].y);
        }

        status = 0;
    } else {

        // TODO
        status = -1;
    }

    SDL_stack_free(final_points);

    return status;
}


#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */

