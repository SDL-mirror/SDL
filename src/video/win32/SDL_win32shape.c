/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2010 Eli Gottlieb

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

    Eli Gottlieb
    eligottlieb@gmail.com
*/

#include "SDL_win32shape.h"
#include "SDL_win32video.h"

SDL_WindowShaper*
Win32_CreateShaper(SDL_Window * window) {
    int resized_properly;
    SDL_WindowShaper* result = (SDL_WindowShaper *)SDL_malloc(sizeof(SDL_WindowShaper));
    result->window = window;
    result->mode.mode = ShapeModeDefault;
    result->mode.parameters.binarizationCutoff = 1;
    result->usershownflag = 0;
    result->driverdata = (SDL_ShapeData*)SDL_malloc(sizeof(SDL_ShapeData));
    ((SDL_ShapeData*)result->driverdata)->mask_tree = NULL;
    //Put some driver-data here.
    window->shaper = result;
    resized_properly = Win32_ResizeWindowShape(window);
    if (resized_properly != 0)
            return NULL;

    return result;
}

typedef struct {
	POINT corners[4];
	void* next;
} SDL_ShapeRect;

void
CombineRectRegions(SDL_ShapeTree* node,void* closure) {
    SDL_ShapeRect* rect_list = *((SDL_ShapeRect**)closure);
    if(node->kind == OpaqueShape) {
        SDL_ShapeRect* rect = SDL_malloc(sizeof(SDL_ShapeRect));
        rect->corners[0].x = node->data.shape.x; rect->corners[0].y = node->data.shape.y;
        rect->corners[1].x = node->data.shape.x + node->data.shape.w; rect->corners[1].y = node->data.shape.y;
        rect->corners[2].x = node->data.shape.x + node->data.shape.w; rect->corners[2].y = node->data.shape.y + node->data.shape.h;
        rect->corners[3].x = node->data.shape.x; rect->corners[3].y = node->data.shape.y + node->data.shape.h;
        rect->next = *((SDL_ShapeRect**)closure);
        *((SDL_ShapeRect**)closure) = rect;
    }
}

Uint32 num_shape_rects(SDL_ShapeRect* rect) {
    if(rect == NULL)
        return 0;
	else
		return 1 + num_shape_rects(rect->next);
}

int
Win32_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode) {
    SDL_ShapeData *data;
    HRGN mask_region;
	SDL_ShapeRect* rects = NULL,*old = NULL;
	Uint16 num_rects = 0,i = 0;
	int* polygonVertexNumbers = NULL;
	POINT* polygons = NULL;

    if (shaper == NULL || shape == NULL)
        return SDL_INVALID_SHAPE_ARGUMENT;
    if(shape->format->Amask == 0 && shapeMode->mode != ShapeModeColorKey || shape->w != shaper->window->w || shape->h != shaper->window->h)
        return SDL_INVALID_SHAPE_ARGUMENT;
    
    data = (SDL_ShapeData*)shaper->driverdata;
    if(data->mask_tree != NULL)
        SDL_FreeShapeTree(&data->mask_tree);
    data->mask_tree = SDL_CalculateShapeTree(*shapeMode,shape);
    
    SDL_TraverseShapeTree(data->mask_tree,&CombineRectRegions,&rects);
    num_rects = num_shape_rects(rects);
    polygonVertexNumbers = (int*)SDL_malloc(sizeof(int)*num_rects);
    for(i=0;i<num_rects;i++)
        polygonVertexNumbers[i] = 4;
    polygons = (POINT*)SDL_malloc(sizeof(POINT)*4*num_rects);
    for(i=0;i<num_rects*4;i++) {
        polygons[i] = rects[i / 4].corners[i % 4];
        if(i % 4 == 3) {
            old = rects;
            rects = rects->next;
            SDL_free(old);
		}
	}

    /*
     * Set the new region mask for the window 
     */
    mask_region = CreatePolyPolygonRgn(polygons,polygonVertexNumbers,num_rects,WINDING);
    SetWindowRgn(((SDL_WindowData *)(shaper->window->driverdata))->hwnd, mask_region, TRUE);

    SDL_free(polygons);
    SDL_free(polygonVertexNumbers);
    
    return 0;
}

int
Win32_ResizeWindowShape(SDL_Window *window) {
    SDL_ShapeData* data;

    if (window == NULL)
        return -1;
    data = (SDL_ShapeData *)window->shaper->driverdata;
    if (data == NULL)
        return -1;
    
    if(data->mask_tree != NULL)
        SDL_FreeShapeTree(&data->mask_tree);
    
    window->shaper->usershownflag |= window->flags & SDL_WINDOW_SHOWN;
    
    return 0;
}
