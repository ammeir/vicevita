
/* control_pad.cpp: Scans changes in buttons, joystick and touch screen.

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

#include "control_pad.h"
#include "controls.h"
#include "vkeyboard.h"
#include "guitools.h"
#include "debug_psv.h"
#include <cstring>
#include <psp2/ctrl.h>
#include <psp2/touch.h>


static int	gs_analogDirectionLookUp[9] = {0, ANALOG_UP, ANALOG_DOWN, 0, ANALOG_LEFT, 0, 0, 0, ANALOG_RIGHT};

ControlPad::ControlPad()
{
}

ControlPad::~ControlPad()
{
}

void ControlPad::init(View* view, Controls* controls, VirtualKeyboard* keyboard)
{
	m_view = view;
	m_controls = controls;
	m_keyboard = keyboard;
	m_joystickMask = 0x00;
	m_realBtnMask = 0;
	m_joystickScanSide = 0;
	
	// Digital buttons + Analog support.
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	
	// Enable front touchscreen
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	//sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
}

void ControlPad::scan(ControlPadMap** maps, int* psize, bool scan_keyboard, bool scan_mouse)
{
	static SceCtrlData ctrl;
	static SceTouchData touch;
	
	char curr_joystick_bits = 0;
	static char prev_joystick_bits = 0;
	static int prevButtonsScan = 0;
	static TouchCoordinates touchBuf[16];
	static ControlPadMap mouseBuf[8];
	static int scan_count = 0;
	
	/* Read controls */
	sceCtrlPeekBufferPositive(0, &ctrl, 1); // ctrl.buttons gives you a bit mask of all the buttons pressed


	/* Convert joystick axis movememts to bits */
	curr_joystick_bits = 0x00;
	int jx = (m_joystickScanSide)? ctrl.rx: ctrl.lx;
	int jy = (m_joystickScanSide)? ctrl.ry: ctrl.ly;

	// X axis
	if (jx <= 40)
		curr_joystick_bits |= 0x04; // ANALOG_LEFT
	else if (jx >= 216)
		curr_joystick_bits |= 0x08; // ANALOG_RIGHT
	// Y axis
	if (jy <= 40){
		curr_joystick_bits |= 0x01; // ANALOG_UP
	}
	else if (jy >= 216)
		curr_joystick_bits |= 0x02; // ANALOG_DOWN


	getMaps(ctrl.buttons, prevButtonsScan, curr_joystick_bits, prev_joystick_bits, maps, psize);

	prevButtonsScan = ctrl.buttons;
	prev_joystick_bits = curr_joystick_bits;


	if (scan_keyboard && !(++scan_count % 2)) {

		scan_count = 0;

		/* Read front touch screen */
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

		// touch.reportNum indicates how many touches we have on the screen
		int touch_count = touch.reportNum;

		for (int i=0; i<touch_count; ++i){
			touchBuf[i].x = touch.report[i].x;
			touchBuf[i].y = touch.report[i].y;	
		}
		
		// These must be called even if touch count is zero because we need to identify key releases.
		m_keyboard->input(touchBuf, touch_count);
		m_keyboard->getKeyMaps(maps, psize);
	}
}

void ControlPad::getMaps(int curr_bmask, int prev_bmask, 
						  char curr_jmask, char prev_jmask,
						  ControlPadMap** maps, int* size)
{
	// Search control pad (buttons and joystick) for mappings.
	// Only return maps if there are new actions e.g. if a new button is pressed or
	// button releases from press state.

	//  Buttons
	int new_actions = prev_bmask ^ curr_bmask;
	if (new_actions){
		ControlPadMap* map;
		int bit_slider = 1;
		while (bit_slider != 0x00010000){ // 16 buttons
			if (new_actions & bit_slider){
				m_realBtnMask ^= bit_slider;
				if(map = m_controls->getMappedKeyDigital(bit_slider, m_realBtnMask)){
					map->ispress = curr_bmask & bit_slider; // Pressed or released}
					*maps++ = map; (*size)++;

					// HACK: Release ltrigger map key after shifting release. 
					// Scenario: 
					// 1. ltrigger is mapped to a key.
					// 2. press ltrigger --> press triangle --> release ltrigger --> release triangle.
					// When releasing ltrigger it actually releases the ltrigger+triangle map and the first 
					// ltrigger map is never released. Release it here.
					if ((bit_slider & SCE_CTRL_LTRIGGER) && !(curr_bmask & bit_slider)){ // Is ltrigger release?
						if (map->ind != SCE_CTRL_LTRIGGER){ // Is released map index other than ltrigger (i.e a shift release)?
							map = m_controls->getMappedKeyDigital(SCE_CTRL_LTRIGGER, 0);
							map->ispress = false;
							*maps++ = map; (*size)++;
						}
					}
				}
			}
			bit_slider <<= 1;
		}
	}

	// Joystick
	new_actions = prev_jmask ^ curr_jmask;
	if (new_actions){
		ControlPadMap* map;
		int bit_slider = 1;
		while (bit_slider != 0x10){
			if (new_actions & bit_slider){
				if (map = m_controls->getMappedKeyAnalog(gs_analogDirectionLookUp[bit_slider])){
					map->ispress = curr_jmask & bit_slider; // Pressed or released
					*maps++ = map; (*size)++;
				}
			}
			bit_slider <<= 1;
		}
	}
}

void ControlPad::changeJoystickScanSide(const char* side)
{
	if (!strcmp(side, "Right"))
		m_joystickScanSide = 1; 
	else if (!strcmp(side, "Left"))
		m_joystickScanSide = 0;
}

void ControlPad::waitTillButtonsReleased()
{
	SceCtrlData ctrl;
	sceCtrlPeekBufferPositive(0, &ctrl, 1); 

	if (ctrl.buttons){
		while(1)
		{
			sceCtrlReadBufferPositive(0, &ctrl, 1);
			if (!ctrl.buttons){
				break;
			}
		}
	}
}