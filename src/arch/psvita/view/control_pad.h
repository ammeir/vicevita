
/* control_pad.h: Scans changes in button and joystick movements.

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


#ifndef CONTROL_PAD_H
#define CONTROL_PAD_H

#include <string>


using std::string;

typedef struct{
	int mid;	 // map value
	int ind;	 // map index
	int iskey;	 // is keyboard key
	int isjoystick;
	int	istouch;
	int	ispress; // Button or touch press/release	
	int joypin;
	int touch_x;
	int touch_y;
} ControlPadMap;


class View;
class Controls;
class VirtualKeyboard;
class  ControlPad
{

private:
	View*			m_view;
	Controls*		m_controls;
	VirtualKeyboard* m_keyboard;
	unsigned char	m_joystickMask;
	int				m_joystickScanSide;
	
	
	// Button scanning bitmask is not reliable with combination maps (e.g L1 + Circle). 
	// Identifying press is easy but the release is tricky. Scan result might show reset for both bits
	// if buttons are released at exactly the same moment. This variable is used to keep track of all 
	// button presses/releases and to help identify the right map.
	int				m_realBtnMask;

	int				touchCoordinatesToButton(int x, int y);
	void			getMaps(int curr_bmask, int prev_bmask, 
							char curr_jmask, char prev_jmask,
							ControlPadMap** maps, int* size);
	
public:
					ControlPad();
	virtual			~ControlPad();

	void			init(View*, Controls*, VirtualKeyboard*);
	void			scan(ControlPadMap** maps, int* size, bool touchScan, bool scan_mouse = false);
	void			changeJoystickScanSide(const char* side);
	void			waitTillButtonsReleased();
};

#endif