/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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
*/
#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_rect_c.h"

SDL_bool
SDL_HasIntersection(const SDL_Rect * A, const SDL_Rect * B)
{
    int Amin, Amax, Bmin, Bmax;

    /* Horizontal intersection */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return SDL_FALSE;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return SDL_FALSE;

    return SDL_TRUE;
}

SDL_bool
SDL_IntersectRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result)
{
    int Amin, Amax, Bmin, Bmax;

    /* Horizontal intersection */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if (Bmin > Amin)
        Amin = Bmin;
    result->x = Amin;
    if (Bmax < Amax)
        Amax = Bmax;
    result->w = Amax - Amin;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if (Bmin > Amin)
        Amin = Bmin;
    result->y = Amin;
    if (Bmax < Amax)
        Amax = Bmax;
    result->h = Amax - Amin;

    return !SDL_RectEmpty(result);
}

void
SDL_UnionRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result)
{
    int Amin, Amax, Bmin, Bmax;

    /* Horizontal union */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if (Bmin < Amin)
        Amin = Bmin;
    result->x = Amin;
    if (Bmax > Amax)
        Amax = Bmax;
    result->w = Amax - Amin;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if (Bmin < Amin)
        Amin = Bmin;
    result->y = Amin;
    if (Bmax > Amax)
        Amax = Bmax;
    result->h = Amax - Amin;
}

void
SDL_AddDirtyRect(SDL_DirtyRectList * list, const SDL_Rect * rect)
{
    SDL_DirtyRect *dirty;

    /* FIXME: At what point is this optimization too expensive? */
    for (dirty = list->list; dirty; dirty = dirty->next) {
        if (SDL_HasIntersection(&dirty->rect, rect)) {
            SDL_UnionRect(&dirty->rect, rect, &dirty->rect);
            return;
        }
    }

    if (list->free) {
        dirty = list->free;
        list->free = dirty->next;
    } else {
        dirty = (SDL_DirtyRect *) SDL_malloc(sizeof(*dirty));
        if (!dirty) {
            return;
        }
    }
    dirty->rect = *rect;
    dirty->next = list->list;
    list->list = dirty;
}

void
SDL_ClearDirtyRects(SDL_DirtyRectList * list)
{
    SDL_DirtyRect *prev, *curr;

    /* Skip to the end of the free list */
    prev = NULL;
    for (curr = list->free; curr; curr = curr->next) {
        prev = curr;
    }

    /* Add the list entries to the end */
    if (prev) {
        prev->next = list->list;
    } else {
        list->free = list->list;
    }
    list->list = NULL;
}

void
SDL_FreeDirtyRects(SDL_DirtyRectList * list)
{
    while (list->list) {
        SDL_DirtyRect *elem = list->list;
        list->list = elem->next;
        SDL_free(elem);
    }
    while (list->free) {
        SDL_DirtyRect *elem = list->free;
        list->free = elem->next;
        SDL_free(elem);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
