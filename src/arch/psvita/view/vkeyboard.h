
/* vkeyboard.h: Implementation of C64 virtual keyboard.

   Copyright (C) 2019-2020 Amnon-Dan Meir.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: 
     Email: ammeir71@yahoo.com
*/

#ifndef VIEW_KEYBOARD_H
#define VIEW_KEYBOARD_H

#include "control_pad.h"
#include <list>

typedef struct{			
	int x;
	int y;
} TouchCoordinates;

typedef struct{
	int x;
	int y;
	int width;
	int height;
}RectCoordinates;

#define KEYBOARD_UP			 0x01
#define KEYBOARD_DOWN		 0x02
#define KEYBOARD_MOVING_UP	 0x04
#define KEYBOARD_MOVING_DOWN 0x08

extern int  g_keyboardStatus;

class View;
class Controls;
class vita2d_texture;
class VirtualKeyboard
{

private:
	View*				m_view;
	std::list<int>		m_touchBuffer; 
	ControlPadMap*		m_keyMapLookup;
	float				m_posX;
	float				m_posY;
	float				m_scaleX;
	float				m_scaleY;
	vita2d_texture*		m_keyboard;
	vita2d_texture*		m_keyboardStd;
	vita2d_texture*		m_keyboardShift;
	vita2d_texture*		m_keyboardCmb;
	vita2d_texture*		m_keyboardCtrl;
	bool				m_shiftLock;
	bool				m_updated;
	bool				m_animation;

	void				changeLayout(int mid);
	int					touchCoordinatesToMid(int x, int y);
	void				midToKeyboardCoordinates(int mid, RectCoordinates* tc);
	void				showMagnifiedKey(int mid);
	void				initAnimation();

public:
						VirtualKeyboard();
						~VirtualKeyboard();

	void				init(View*, Controls*);
	void				input(TouchCoordinates* touches, int count);
	void				getKeyMaps(ControlPadMap** maps, int* size);
	void				show();
	void				render();
	void				setPosition(int x, int y, float scaleX, float scaleY);
	void				clear();
	bool				isUpdated();
	void				setAnimation(bool);
	void				toggleVisibility();
};

#endif