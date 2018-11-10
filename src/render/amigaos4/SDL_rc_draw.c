/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

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
#include "SDL_rc_draw.h"

#include <proto/graphics.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

typedef struct {
    int x1, y1, x2, y2;
    Uint32 (*blendfp)(Uint32, Uint8, Uint8, Uint8, Uint8);
    Uint32 *baseaddress;
    Uint32 width;
    SDL_bool last;
    Uint8 sr;
    Uint8 sg;
    Uint8 sb;
    Uint8 sa;
} OS4_LineData;

#define ABS(a) (((a) < 0) ? -(a) : (a))
#define ROUNDF(a) (int)((a) + 0.5f)

static inline Uint32*
OS4_GetMemoryAddress(Uint32 * memory, Uint32 width, Uint16 x, Uint16 y)
{
    return memory + x + y * width;
}

static inline Uint32
OS4_GetPixel(Uint32 * memory, Uint32 width, Uint16 x, Uint16 y)
{
    return *(memory + x + y * width);
}

static inline void
OS4_SetPixel(Uint32 * memory, Uint32 width, Uint16 x, Uint16 y, Uint32 color)
{
    *(memory + x + y * width) = color;
}

static inline Uint8
ALPHA(Uint32 c)
{
    return (c >> 24);
}

static inline Uint8
RED(Uint32 c)
{
    return (c >> 16);
}

static inline Uint8
GREEN(Uint32 c)
{
    return (c >> 8);
}

static inline Uint8
BLUE(Uint32 c)
{
    return c;
}

static inline Uint8
MUL(Uint8 a, Uint8 b)
{
    Uint32 u = (a * b) / 255;
    return u;
}

static inline Uint8
ADD(Uint8 a, Uint8 b)
{
    Uint32 c = a + b;

    if (c > 255) c = 255;

    return c;
}

static Uint32
BlendPoint(Uint32 old, Uint8 sr, Uint8 sg, Uint8 sb, Uint8 sa)
{
    Uint8 dr, dg, db, da, one_minus_alpha;

    one_minus_alpha = 255 - sa;

    dr = sr + MUL(one_minus_alpha, RED(old));
    dg = sg + MUL(one_minus_alpha, GREEN(old));
    db = sb + MUL(one_minus_alpha, BLUE(old));
    da = sa + MUL(one_minus_alpha, ALPHA(old));

    return da << 24 | dr << 16 | dg << 8 | db;
}

static Uint32
AddPoint(Uint32 old, Uint8 sr, Uint8 sg, Uint8 sb, Uint8 sa)
{
    Uint8 dr, dg, db, da;

    dr = ADD(sr, RED(old));
    dg = ADD(sg, GREEN(old));
    db = ADD(sb, BLUE(old));
    da = ALPHA(old);

    return da << 24 | dr << 16 | dg << 8 | db;
}

static Uint32
ModPoint(Uint32 old, Uint8 sr, Uint8 sg, Uint8 sb, Uint8 sa)
{
    Uint8 dr, dg, db, da;

    dr = MUL(sr, RED(old));
    dg = MUL(sg, GREEN(old));
    db = MUL(sb, BLUE(old));
    da = ALPHA(old);

    return da << 24 | dr << 16 | dg << 8 | db;
}

static Uint32
NopPoint(Uint32 old, Uint8 sr, Uint8 sg, Uint8 sb, Uint8 sa)
{
    return old;
}

int
OS4_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                    int count, SDL_BlendMode mode, Uint8 a, Uint8 r, Uint8 g, Uint8 b)
{
    OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
    struct BitMap *bitmap = OS4_ActivateRenderer(renderer);
    int i, status;

    int minx, miny, maxx, maxy;

    if (!bitmap) {
        return -1;
    }

    minx = data->cliprect.x;
    miny = data->cliprect.y;
    maxx = data->cliprect.x + data->cliprect.w - 1;
    maxy = data->cliprect.y + data->cliprect.h - 1;

    if (mode == SDL_BLENDMODE_NONE) {

        int32 ret = 0;

        const Uint32 color = a << 24 | r << 16 | g << 8 | b;

        for (i = 0; i < count; ++i) {

            int x, y;

            x = points[i].x;
            y = points[i].y;

            /* Clipping - is it possible clip with RastPort? */
            if (x < minx || x > maxx || y < miny || y > maxy) {
                continue;
            }

            ret |= data->iGraphics->WritePixelColor(
                &data->rastport,
                x,
                y,
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

            Uint32 (*blendfp)(Uint32, Uint8, Uint8, Uint8, Uint8);

            const Uint32 width = bytesperrow / 4;
            Uint8 sr, sg, sb, sa;

            switch (mode) {
                case SDL_BLENDMODE_BLEND:
                    sr = MUL(a, r);
                    sg = MUL(a, g);
                    sb = MUL(a, b);
                    sa = a;
                    blendfp = BlendPoint;
                    break;
                case SDL_BLENDMODE_ADD:
                    sr = MUL(a, r);
                    sg = MUL(a, g);
                    sb = MUL(a, b);
                    sa = 0;
                    blendfp = AddPoint;
                    break;
                case SDL_BLENDMODE_MOD:
                    sr = r;
                    sg = g;
                    sb = b;
                    sa = 0;
                    blendfp = ModPoint;
                    break;
                default:
                    dprintf("Unknown blend mode %d\n", mode);
                    sr = r;
                    sg = g;
                    sb = b;
                    sa = a;
                    blendfp = NopPoint;
                    break;
            }

            for (i = 0; i < count; ++i) {
                Uint32 newcolor, oldcolor;
                int x, y;

                x = points[i].x;
                y = points[i].y;

                /* Clipping */
                if (x < minx || x > maxx || y < miny || y > maxy) {
                    continue;
                }

                oldcolor = OS4_GetPixel(baseaddress, width, x, y);

                newcolor = blendfp(oldcolor, sr, sg, sb, sa);

                OS4_SetPixel(baseaddress, width, x, y, newcolor);
            }

            data->iGraphics->UnlockBitMap(lock);

            status = 0;
        } else {
            dprintf("Lock failed\n");
            status = -1;
        }
    }

    return status;
}

static void
OS4_HLine(OS4_LineData * data)
{
    int x, minx, maxx;
    Uint32 *memory;

    if (data->x1 < data->x2) {
        minx = data->x1;
        maxx = data->x2;

        if (!data->last) {
            --maxx;
        }
    } else {
        minx = data->x2;
        maxx = data->x1;

        if (!data->last) {
            ++minx;
        }
    }

    memory = OS4_GetMemoryAddress(data->baseaddress, data->width, minx, data->y1);

    for (x = minx; x <= maxx; ++x) {
        Uint32 oldcolor, newcolor;

        oldcolor = *memory;
        newcolor = data->blendfp(oldcolor, data->sr, data->sg, data->sb, data->sa);
        *memory++ = newcolor;
    }
}

static void
OS4_VLine(OS4_LineData * data)
{
    int y, miny, maxy;
    Uint32 *memory;

    if (data->y1 < data->y2) {
        miny = data->y1;
        maxy = data->y2;

        if (!data->last) {
            --maxy;
        }
    } else {
        miny = data->y2;
        maxy = data->y1;

        if (!data->last) {
            ++miny;
        }
    }

    memory = OS4_GetMemoryAddress(data->baseaddress, data->width, data->x1, miny);

    for (y = miny; y <= maxy; ++y) {
        Uint32 oldcolor, newcolor;

        oldcolor = *memory;
        newcolor = data->blendfp(oldcolor, data->sr, data->sg, data->sb, data->sa);
        *memory = newcolor;
        memory += data->width;
    }
}

static void
OS4_DLine(OS4_LineData * data)
{
    int x, y, startx, starty, endx, endy, ystep, width;
    Uint32 *memory;

    if (data->x1 < data->x2) {
        startx = data->x1;
        starty = data->y1;

        endx = data->x2;
        endy = data->y2;

        if (!data->last) {
            //--endx;
        }
    } else {
        startx = data->x2;
        starty = data->y2;

        endx = data->x1;
        endy = data->y1;

        if (!data->last) {
            //++startx;
        }
    }

    if (endy < starty) {
        ystep = -1;
        width = -data->width;
    } else {
        ystep = 1;
        width = data->width;
    }

    memory = OS4_GetMemoryAddress(data->baseaddress, data->width, startx, starty);

    //dprintf("%d, %d -> %d, %d, ystep %d\n", data->x1, data->y1, data->x2, data->y2, ystep);

    for (x = startx, y = starty; x <= endx; ++x, y += ystep) {
        Uint32 oldcolor, newcolor;

        oldcolor = *memory;
        newcolor = data->blendfp(oldcolor, data->sr, data->sg, data->sb, data->sa);
        *memory++ = newcolor;
        memory += width;
    }
}

static void
OS4_Line(OS4_LineData *data)
{
    float ystep;
    int x, y, startx, starty, endx, endy;
    Uint32 *memory;

    if (data->x1 < data->x2) {
        startx = data->x1;
        starty = data->y1;

        endx = data->x2;
        endy = data->y2;
    } else {
        startx = data->x2;
        starty = data->y2;

        endx = data->x1;
        endy = data->y1;
    }

    // TODO: last
    ystep = (float)(endy - starty) / (float)(endx - startx);

    //dprintf("%d, %d -> %d, %d, ystep %d\n", data->x1, data->y1, data->x2, data->y2, (int)(10 * ystep));

    memory = OS4_GetMemoryAddress(data->baseaddress, data->width, startx, starty);

    if (ystep > 0.5f || ystep < -0.5f) {

        float xstep = 1.0f / ystep;
        float fx = startx;

        int lastx = startx;

        for (y = starty ; y <= endy; fx += xstep, ++y) {

            Uint32 oldcolor, newcolor;

            x = ROUNDF(fx);

            oldcolor = *memory;
            newcolor = data->blendfp(oldcolor, data->sr, data->sg, data->sb, data->sa);
            *memory = newcolor;
            memory += data->width;

            memory += (x - lastx);
            lastx = x;
        }

    } else {
        float fy = starty;

        int lasty = starty;

        for (x = startx ; x <= endx; ++x, fy += ystep ) {
            Uint32 oldcolor, newcolor;
            int diff;

            y = ROUNDF(fy);

            oldcolor = *memory;
            newcolor = data->blendfp(oldcolor, data->sr, data->sg, data->sb, data->sa);
            *memory++ = newcolor;

            diff = y - lasty;
            if (diff < 0) {
                memory -= data->width;
                lasty = y;
            } else if (diff > 0) {
                memory += data->width;
                lasty = y;
            }
        }
    }
}

static void
OS4_BlendLine(OS4_LineData * data)
{
    if (data->y1 == data->y2) {
        OS4_HLine(data);
    } else if (data->x1 == data->x2) {
        OS4_VLine(data);
    } else if (ABS(data->x1 - data->x2) == ABS(data->y1 - data->y2)) {
        OS4_DLine(data);
    } else {
        OS4_Line(data);
    }
}

int
OS4_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                    int count, SDL_BlendMode mode, Uint8 a, Uint8 r, Uint8 g, Uint8 b)
{
    OS4_RenderData *data = (OS4_RenderData *) renderer->driverdata;
    struct BitMap *bitmap = OS4_ActivateRenderer(renderer);

    int i, status;

    if (!bitmap) {
        return -1;
    }

    if (mode == SDL_BLENDMODE_NONE) {

        const Uint32 color = a << 24 | r << 16 | g << 8 | b;

        data->iGraphics->SetRPAttrs(&data->rastport, RPTAG_APenColor, color, TAG_DONE);

        for (i = 0; i < count - 1; ++i) {

            int x1, y1, x2, y2;

            x1 = points[i].x;
            y1 = points[i].y;
            x2 = points[i + 1].x;
            y2 = points[i + 1].y;

            /* Clipping - is it possible to do with RastPort? */
            if (!SDL_IntersectRectAndLine(&data->cliprect, &x1, &y1, &x2, &y2)) {
                continue;
            }

            data->iGraphics->Move(
                &data->rastport,
                x1,
                y1);

            data->iGraphics->Draw(
                &data->rastport,
                x2,
                y2);
        }

        status = 0;
    } else {

        APTR baseaddress;
        uint32 bytesperrow;

        APTR lock = data->iGraphics->LockBitMapTags(
            bitmap,
            LBM_BaseAddress, &baseaddress,
            LBM_BytesPerRow, &bytesperrow,
            TAG_DONE);

        if (lock) {

            OS4_LineData ld;
            ld.baseaddress = baseaddress;
            ld.width = bytesperrow / 4;
            ld.last = SDL_FALSE;

            switch (mode) {
                case SDL_BLENDMODE_BLEND:
                    ld.sr = MUL(a, r);
                    ld.sg = MUL(a, g);
                    ld.sb = MUL(a, b);
                    ld.sa = a;
                    ld.blendfp = BlendPoint;
                    break;
                case SDL_BLENDMODE_ADD:
                    ld.sr = MUL(a, r);
                    ld.sg = MUL(a, g);
                    ld.sb = MUL(a, b);
                    ld.sa = 0;
                    ld.blendfp = AddPoint;
                    break;
                case SDL_BLENDMODE_MOD:
                    ld.sr = r;
                    ld.sg = g;
                    ld.sb = b;
                    ld.sa = a;
                    ld.blendfp = ModPoint;
                    break;
                default:
                    dprintf("Unknown blend mode %d\n", mode);
                    ld.sr = r;
                    ld.sg = g;
                    ld.sb = b;
                    ld.sa = a;
                    ld.blendfp = NopPoint;
                    break;
            }

            for (i = 0; i < count - 1; ++i) {

                ld.x1 = points[i].x;
                ld.y1 = points[i].y;
                ld.x2 = points[i + 1].x;
                ld.y2 = points[i + 1].y;

                /* Clipping */
                if (!SDL_IntersectRectAndLine(&data->cliprect, &ld.x1, &ld.y1, &ld.x2, &ld.y2)) {
                    continue;
                }

                if (i == count - 2) {
                    ld.last = SDL_TRUE;
                }

                OS4_BlendLine(&ld);
            }

            data->iGraphics->UnlockBitMap(lock);

            status = 0;
        } else {
            dprintf("Lock failed\n");
            status = -1;
        }

    }

    return status;
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */

