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

#include <windows.h>
#include "SDL_win32shape.h"

SDL_WindowShaper* Win32_CreateShaper(SDL_Window * window) {
	SDL_WindowShaper* result = malloc(sizeof(SDL_WindowShaper));
	result->window = window;
	result->alphacutoff = 0;
	result->usershownflag = 0;
	//Put some driver-data here.
	window->shaper = result;
	int resized_properly = X11ResizeWindowShape(window);
	assert(resized_properly == 0);
	return result;
}

int Win32_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode) {
	assert(shaper != NULL && shape != NULL);
	if(!SDL_ISPIXELFORMAT_ALPHA(SDL_MasksToPixelFormatEnum(shape->format->BitsPerPixel,shape->format->Rmask,shape->format->Gmask,shape->format->Bmask,shape->format->Amask)))
		return -2;
	if(shape->w != shaper->window->w || shape->h != shaper->window->h)
		return -3;
	
	/* Assume that shaper->alphacutoff already has a value, because SDL_SetWindowShape() should have given it one. */
	/*
	 * Start with empty region 
	 */
	HRGN MaskRegion = CreateRectRgn(0, 0, 0, 0);
	
	unsigned int pitch = shape->pitch;
	unsigned int width = shape->width;
	unsigned int height = shape->height;
	unsigned int dy = pitch - width;
	
	SDL_ShapeData *data = (SDL_ShapeData*)shaper->driverdata;
	/*
	 * Transfer binarized mask image into workbuffer 
	 */
	SDL_CalculateShapeBitmap(shaper->alphacutoff,shape,data->shapebuffer,1,0xff);
	
	//Move code over to here from AW_windowShape.c
	Uint8 *pos1 = data->shapebuffer + width - 1;
	Uint8 *pos2 = (Uint8*) pos1 + 1;
	int x = 0,y = 0;
	int visible = 0;
	int vxmin = shape->width - 1;
	int vxmax = -1;
	int vymin = shape->height - 1;
	int vymax = -1;
	for (y = 0; y <height; y++) {
		Uint8 inside = 0;
		for (x = -1; x <width; x++) {
			int       newtargetcount;
			POINT     newtarget[5];
			/*
			 * Define local variables 
			 */
			int       newtargetcount;
			POINT     newtarget[5];
			

			/*
			 * Update visible region 
			 */
			if (*curpos)
				visible = 1;
			/*
			 * Determine visible bounds 
			 */
			if (x < vxmin)
				vxmin = x;
			if (x > vxmax)
				vxmax = x;
			if (y < vxymin)
				vxymin = y;
			if (y > vymax) 
				vymax = y;
				
			/*
			 * Check for starting point 
			 */
			Uint8 *TL, *TR, *BL, *BR;
			int target_x, target_y, lasttarget_x, lasttarget_y;
			if (((!*curpos) && (*curpos2 == 0xFF)) || ((!*curpos2) && (*curpos == 0xFF))) {
				if (!*curpos) {
					BR = curpos2;
					BL = (Uint8 *) (BR - 1);
					TR = (Uint8 *) (BR - width);
					TL = (Uint8 *) (TR - 1);
					target_x = x;
					target_y = y;
				}
				else {
					BR = curpos2 + 1;
					BL = (Uint8 *) (BR - 1);
					TR = (Uint8 *) (BR - width);
					TL = (Uint8 *) (TR - 1);
					target_x = x + 1;
					target_y = y;
				}
				
				lasttarget_x = 0;
				lasttarget_y = 0;
				int firsttime = 1;
				pos_array_pos = 0;
				
				while ((target_x != x) || (target_y != y) || firsttime) {
					/*
					 * New array index 
					 */
					firsttime = 0;
					pos_array_pos++;
					/*
					 * Check array index 
					 */
					if (pos_array_pos >= 4096) {
						SDL_SetError("Exceeded maximum number of polygon points.");
						pos_array_pos--;
					}

					/*
					 * Store point in array 
	 				 */
					pos_array[pos_array_pos].x = target_x + 1;
					pos_array[pos_array_pos].y = target_y;

					/*
					 * Mark the four poles as visited 
					 */
					if (*TL)
						*TL = 0x99;
					if (*BL)
						*BL = 0x99;
					if (*TR)
						*TR = 0x99;
					if (*BR)
						*BR = 0x99;
			
					newtargetcount = 0;
					if ((*TL || *TR) && (*TL != *TR)) {
						newtargetcount++;
						newtarget[newtargetcount].x = 0;
						newtarget[newtargetcount].y = -1;
					}

					if ((*TR || *BR) && (*TR != *BR)) {
						newtargetcount++;
						newtarget[newtargetcount].x = 1;
						newtarget[newtargetcount].y = 0;
					}

					if ((*BL || *BR) && (*BL != *BR)) {
						newtargetcount++;
						newtarget[newtargetcount].x = 0;
	 					newtarget[newtargetcount].y = 1;
					}

					if ((*TL || *BL) && (*TL != *BL)) {
						newtargetcount++;
						newtarget[newtargetcount].x = -1;
						newtarget[newtargetcount].y = 0;
					}
				
					switch (newtargetcount) {
						case 1:
							SDL_SetError("Cropping error - Newtargetcount=1.");
							return (-1);
							break;

						case 2:
							if ((target_x + newtarget[1].x != lasttarget_x) || (target_y + newtarget[1].y != lasttarget_y)) {
								lasttarget_x = target_x;
								lasttarget_y = target_y;
								target_x = target_x + newtarget[1].x;
								target_y = target_y + newtarget[1].y;
							}
							else {
								lasttarget_x = target_x;
								lasttarget_y = target_y;
								target_x = target_x + newtarget[2].x;
								target_y = target_y + newtarget[2].y;
							}
							break;

						case 3:
							SDL_SetError("Cropping error - Newtargetcount=3.");
							return (-1);
							break;

						case 4:
							if (lasttarget_x > target_x) {
								lasttarget_x = target_x;
								lasttarget_y = target_y;
								if (*TR != 0x00)
									target_y--;
								else
									target_y++;
							}
							else if (lasttarget_y > target_y) {
								lasttarget_x = target_x;
								lasttarget_y = target_y;
								if (*BL != 0x00)
									target_x--;
								else
									target_x++;
							}
							else if (lasttarget_x < target_x) {
								lasttarget_x = target_x;
								lasttarget_y = target_y;
								if (*TL != 0x00)
									target_y--;
								else
									target_y++;
							}
							else if (lasttarget_y < target_y) {
								lasttarget_x = target_x;
								lasttarget_y = target_y;
								if (*TL != 0x00)
									target_x--;
								else
									target_x++;
							}
							else {
								SDL_SetError("Cropping error - no possible targets on newtargetcount=4.");
								return (-1);
							}
							break;

						default:
							SDL_SetError("Cropping error - Newtargetcount invalid.");
							return (-1);
							break;
					}

					if (target_x > lasttarget_x)
						TL = (Uint8 *) (TL + 1);
					else if (target_x < lasttarget_x)
						TL = (Uint8 *) (TL - 1);
					else if (target_y > lasttarget_y)
						TL = (Uint8 *) (TL + width);
					else if (target_y < lasttarget_y)
						TL = (Uint8 *) (TL - width);
					else {
						SDL_SetError("Cropping error - no new target.");
						return (-1);
					}

					BL = (Uint8 *) (TL + width);
					TR = (Uint8 *) (TL + 1);
					BR = (Uint8 *) (BL + 1);
				}			// End of while loop
			
				/*
				 * Apply the mask to the cropping region 
				 */
				if (pos_array_pos >= 4) {
					TempRegion = CreatePolygonRgn(&(pos_array[1]), pos_array_pos, WINDING);
					if (TempRegion == NULL) {
						SDL_SetError("Cropping error - unable to create polygon.");
						return (-1);
					}

	  				/*
	  				 * Add current region to final mask region 
	  				 */
	  				if (inside)
	  					CombineRgn(MaskRegion, MaskRegion, TempRegion, RGN_DIFF);
					else
						CombineRgn(MaskRegion, MaskRegion, TempRegion, RGN_OR);

					/*
					 * Remove temporary region 
					 */
					DeleteObject(TempRegion);
				}

				/*
				 * Switch sides 
				 */
				inside = !inside;
			}
			else if ((*curpos) && (!*curpos2))
				inside = !inside;
      			else if ((!*curpos) && (*curpos2))
				inside = !inside;
			
			curpos++;
			curpos2++;
		}
		curpos = (Uint8 *) (curpos + 2 * enlarge_mask - 1);
		curpos2 = (Uint8 *) (curpos + 1);
	}
	
	/*
	 * Set the new region mask for the window 
	 */
	SetWindowRgn((SDL_WindowData*)(shaper->window->driverdata)->hwnd, MaskRegion, TRUE);

	/*
	 * Return value 
	 */
	return (0);
}

int Win32_ResizeWindowShape(SDL_Window *window) {
	SDL_ShapeData* data = window->shaper->driverdata;
	assert(data != NULL);
	
	unsigned int buffersize = window->w * window->h;
	if(data->buffersize != buffersize || data->shapebuffer == NULL) {
		data->buffersize = buffersize;
		if(data->shapebuffer != NULL)
			free(data->shapebuffer);
		data->shapebuffer = malloc(data->buffersize);
		if(data->shapebuffer == NULL) {
			SDL_SetError("Could not allocate memory for shaped-window bitmap.");
			return -1;
		}
	}
	
	window->shaper->usershownflag = window->flags & SDL_WINDOW_SHOWN;
	
	return 0;
}
