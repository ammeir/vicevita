
/* navigator.cpp: User interface navigation base class
                  Derive this class whenever you need user input on your gui widget.

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

#include "navigator.h"
#include "debug_psv.h"
#include <psp2/ctrl.h>
#include <psp2/touch.h>


Navigator::Navigator()
{
	m_holdDownCount = 0;
	m_boostMode = false;
	m_btnRepeatSpeed = 3;
	m_joyRepeatSpeed = 1;
	m_prevButtonScan = 0;
	m_prevJoystickBits = 0;
	m_joyPinMask = 0x03; // Joystick up/down enabled by default
}

Navigator::~Navigator()
{
}

void Navigator::scanCyclic()
{
	SceCtrlData ctrl;
	char curr_joystick_bits = 0;

	waitTillButtonsReleased();

	m_running = true;

	while(m_running)
	{
		/* Read controls */
		sceCtrlReadBufferPositive(0, &ctrl, 1); // Blocking read. ctrl.buttons gives you a bit mask of all the buttons pressed
		
		/* Buttons */

		// Iterate through the bitmask to search all the values.
		unsigned int bit_slider = 1;
		while (bit_slider != 0x00010000){ // 16 buttons

			if ((ctrl.buttons & bit_slider) && !(m_prevButtonScan & bit_slider)) // Button pressed
				buttonDown(bit_slider);

			if (!(ctrl.buttons & bit_slider) && (m_prevButtonScan & bit_slider)) // Button released
				buttonUp(bit_slider);

			if ((ctrl.buttons & bit_slider) && (m_prevButtonScan & bit_slider)) // Button hold
				buttonHold(bit_slider);

			// shift to next bit
			bit_slider <<= 1;
		} 

		m_prevButtonScan = ctrl.buttons;


		/* Analog left stick */

		curr_joystick_bits = 0;
		// X axis
		if (ctrl.lx <= 38)
			curr_joystick_bits |= 0x04; // ANALOG_LEFT
		else if (ctrl.lx >= 218)
			curr_joystick_bits |= 0x08; // ANALOG_RIGHT
		// Y axis
		if (ctrl.ly <= 38){
			curr_joystick_bits |= 0x01; // ANALOG_UP
		}
		else if (ctrl.ly >= 218)
			curr_joystick_bits |= 0x02; // ANALOG_DOWN

		bit_slider = 1;
		while (bit_slider != 0x0000010){ // 4 directions

			if (!(m_joyPinMask & bit_slider)){
				bit_slider <<= 1;
				continue;
			}

			if ((curr_joystick_bits & bit_slider) && !(m_prevJoystickBits & bit_slider)) // Joystick pressed
				buttonDown(joypinToButton(bit_slider));

			if (!(curr_joystick_bits & bit_slider) && (m_prevJoystickBits & bit_slider)) // Joystick released
				buttonUp(joypinToButton(bit_slider));

			if ((curr_joystick_bits & bit_slider) && (m_prevJoystickBits & bit_slider)) // Joystick hold
				buttonHold(joypinToButton(bit_slider), NAV_TYPE_JOYSTICK);

			// shift to next bit
			bit_slider <<= 1;
		} 

		m_prevJoystickBits = curr_joystick_bits;
	}
}

void Navigator::setNavJoyPins(int pins)
{
	m_joyPinMask = pins;
}

void Navigator::buttonDown(int button)
{
	switch (button)
	{
		case SCE_CTRL_DOWN:
			navigateDown();
			break;
		case SCE_CTRL_UP:
			navigateUp();
			break;
		case SCE_CTRL_LEFT:
			navigateLeft();
			break;
		case SCE_CTRL_RIGHT:
			navigateRight();
	}

	buttonPressed(button);
}

void Navigator::buttonUp(int button)
{
	if (isExit(button))
		m_running = false;

	m_holdDownCount = 0;
	m_boostMode = false;
	buttonReleased(button);
}

void Navigator::buttonHold(int button, NavInputType type)
{
	if (m_boostMode){

		if (!isRepeatTime(type))
			return;

		switch (button)
		{
		case SCE_CTRL_DOWN:
			navigateDown();
			break;
		case SCE_CTRL_UP:
			navigateUp();
			break;
		case SCE_CTRL_LEFT:
			navigateLeft();
			break;
		case SCE_CTRL_RIGHT:
			navigateRight();
		}
	}
	else{
		if (++m_holdDownCount > 10)
		m_boostMode = true;
	}
}

bool Navigator::isRepeatTime(NavInputType type)
{
	static int counter = 0;
	static int repeat_speed = 0;

	repeat_speed = (type == NAV_TYPE_BUTTON)? m_btnRepeatSpeed: m_joyRepeatSpeed;

	if (++counter >= repeat_speed){
		counter = 0;
		return true;
	}

	return false;
}

int Navigator::joypinToButton(int joy_pin)
{
	int button = 0;
	switch (joy_pin)
	{
		case 0x01: button = SCE_CTRL_UP; break;
		case 0x02: button = SCE_CTRL_DOWN; break;
		case 0x04: button = SCE_CTRL_LEFT; break;
		case 0x08: button = SCE_CTRL_RIGHT;
	}

	return button;
}

void Navigator::waitTillButtonsReleased()
{
	SceCtrlData ctrl;

	sceCtrlPeekBufferPositive(0, &ctrl, 1); 

	if (ctrl.buttons){
		//DEBUG("Bunttons down on init. Wait till released");
		while(1)
		{
			sceCtrlReadBufferPositive(0, &ctrl, 1);
			if (!ctrl.buttons){
				//DEBUG("Released!");
				break;
			}
		}
	}
}
