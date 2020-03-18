
/* statusbar.h: Statusbar implementation.

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

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <string>

#define STATUSBAR_SPEED 1
#define STATUSBAR_TAPE  2
#define STATUSBAR_PAUSE 4


using std::string;

class View;
class Statusbar
{

private:
	View*	m_view;
	int		m_fps;
	int		m_cpuPercentage;
	int		m_warpFlag;
	string	m_tapeCounter;
	string	m_tapeControl;
	int		m_displaySpeedData;
	int		m_displayTapeData;
	int		m_displayPause;
	bool	m_updated;

public:
			Statusbar();
			~Statusbar();

	void	init(View*);
	void	show();
	void	render();
	void	showStatus(int type, int val);
	void	setSpeedData(int fps, int percent, int warp_flag);
	void	setTapeCounter(int counter);
	void	setTapeControl(int control);
	bool	isUpdated();
};

#endif