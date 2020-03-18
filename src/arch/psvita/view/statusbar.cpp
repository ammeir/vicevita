
/* statusbar.cpp: Statusbar implementation.

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

#include "statusbar.h"
#include "view.h"
#include "texter.h"
#include "app_defs.h"
#include <vita2d.h>


Statusbar::Statusbar()
{
	m_displaySpeedData = 0;
	m_displayTapeData = 0;
	m_displayPause = 0;
	m_updated = false;
}

Statusbar::~Statusbar()
{

}

void Statusbar::init(View* view)
{
	m_view = view;
}

void Statusbar::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing();
    vita2d_swap_buffers();
}

void Statusbar::render()
{
	static char buf[128];
	static char perc[8];

	// Frame
	vita2d_draw_rectangle(0, 513, 960, 31, BLACK);
	// Background
	vita2d_draw_rectangle(0, 515, 960, 27, DARK_GREY2);
	// Dividers
	vita2d_draw_rectangle(268, 513, 2, 31, BLACK); 
	vita2d_draw_rectangle(648, 513, 2, 31, BLACK); 


	if (m_displayTapeData){
		sprintf(buf, "Tape: %s, Counter: %s", m_tapeControl.c_str(), m_tapeCounter.c_str());
		txtr_draw_text(10, 534, YELLOW, buf);
	}

	if (m_displaySpeedData){	
		m_cpuPercentage < 100? sprintf(perc, "0%d", m_cpuPercentage): sprintf(perc, "%d", m_cpuPercentage);
		sprintf(buf, "FPS: %d,  CPU: %s%%,  Warp mode: %d", m_fps, perc, m_warpFlag);
		txtr_draw_text(278, 534, YELLOW, buf);
	}

	if (m_displayPause){
		txtr_draw_text(873, 534, YELLOW, "Paused");
	}

	m_updated = false;
}

void Statusbar::showStatus(int type, int val)
{
	switch (type){
	case STATUSBAR_SPEED:
		m_displaySpeedData = val;
		break;
	case STATUSBAR_TAPE:
		m_displayTapeData = val;
		break;
	case STATUSBAR_PAUSE:
		m_displayPause = val;
		break;
	}
}

void Statusbar::setSpeedData(int fps, int percent, int warp_flag)
{
	m_fps = fps;
	m_cpuPercentage = percent;
	m_warpFlag = warp_flag;
	m_updated = true;
}

void Statusbar::setTapeCounter(int counter)
{
	char buf[8] = {0};
	switch (counter){
	case 0 ... 9:
		sprintf(buf, "00%d", counter);
		break;
	case 10 ... 99:
		sprintf(buf, "0%d", counter);
		break;
	case 100 ... 999:
		sprintf(buf, "%d", counter);
		break;
	default:
		sprintf(buf, "000", counter);
		break;
	}
	
	m_tapeCounter = buf;
	m_updated = true;
}

void Statusbar::setTapeControl(int control)
{
	switch (control){
	case 0:
		m_tapeControl = "Stop"; 
		break;
	case 1: 
		m_tapeControl = "Play"; 
		break;
	case 2: 
		m_tapeControl = "Forward"; 
		break;
	case 3: 
		m_tapeControl = "Rewind"; 
		break;
	case 4: 
		m_tapeControl = "Record"; 
		break;
	case 5: 
		m_tapeControl = "Reset";
		break;
	default: 
		m_tapeControl = "Stop";
	}
	// Send update to peripherals too. 
	m_view->onSettingChanged(DATASETTE_CONTROL, m_tapeControl.c_str(),0,0,0,1);
	m_updated = true;
}

bool Statusbar::isUpdated()
{
	return m_updated;
}

