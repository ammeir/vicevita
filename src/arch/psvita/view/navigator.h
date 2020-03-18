
/* navigator.h: User interface navigation base class
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

#ifndef NAVIGATOR_H
#define NAVIGATOR_H

enum NavInputType {NAV_TYPE_BUTTON = 0, NAV_TYPE_JOYSTICK, NAV_TYPE_TOUCH};

class Navigator
{

private:
	int				m_holdDownCount;
	int				m_btnRepeatSpeed;
	int				m_joyRepeatSpeed;
	bool			m_boostMode;
	int				m_joyPinMask; // Mask of allowed joystick pins.
	int				m_prevButtonScan;
	char			m_prevJoystickBits;
	
	void			buttonUp(int button);
	void			buttonDown(int button);
	void			buttonHold(int button, NavInputType type = NAV_TYPE_BUTTON);
	bool			isRepeatTime(NavInputType type);
	int				joypinToButton(int joy_pin);
	void			waitTillButtonsReleased();

	virtual void	navigateUp(){};
	virtual void	navigateDown(){};
	virtual void	navigateLeft(){};
	virtual void	navigateRight(){};
	virtual void    buttonPressed(int button){};
	virtual void    buttonReleased(int button){};
	virtual bool	isExit(int buttons){return false;};
	

protected:
	bool			m_running;
	
	void			setNavJoyPins(int pins);
	
public:
	
					Navigator();
	virtual			~Navigator();

	void			scanCyclic();
};

#endif