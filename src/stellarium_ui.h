/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�reau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Module to manage User Interface

#ifndef _STELLARIUM_UI_H
#define _STELLARIUM_UI_H

#include "SDL.h"

void initUi(void);
void clearUi(void);
void renderUi();
void GuiHandleClic(Uint16 x, Uint16 y, Uint8 state, Uint8 button);
void GuiHandleMove(int x, int y);
bool GuiHandleKeys(SDLKey key, int state);
void GuiHandleMove(Uint16 x, Uint16 y);

#endif  //_STELLARIUM_UI_H
