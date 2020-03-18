
/* about.cpp: About window.

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

#include "about.h"
#include "view.h"
#include "texter.h"
#include "app_defs.h"
#include "resources.h"

#include <vector>
#include <vita2d.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>


About::About()
{
	m_rainbowLogo = NULL;
}

About::~About()
{
	if(m_rainbowLogo)
		vita2d_free_texture(m_rainbowLogo);
}

void About::doModal()
{
	show();
	scanCyclic();	
}

void About::init()
{
	m_rainbowLogo = vita2d_load_PNG_buffer(img_rainbow_logo);
}

void About::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	// Top seperation line
	vita2d_draw_line(20, 40, 940, 40, YELLOW_TRANSPARENT);

	vita2d_draw_texture(m_rainbowLogo, 325, 170);
		
	txtr_draw_text(400, 190, YELLOW, "VICE VITA C64  v.1.0");
	txtr_draw_text(325, 240, YELLOW, "Commodore 64 emulator written by:");
	txtr_draw_text(325, 270, YELLOW, "2019-2020   Amnon-Dan Meir.");
	txtr_draw_text(325, 300, YELLOW, "1998-2018   VICE team.");
	txtr_draw_text(325, 345, YELLOW, "For additional information:");
	txtr_draw_text(325, 375, C64_BLUE, "@ammeir71");
	
	// Bottom seperation line
	vita2d_draw_line(20, 495, 940, 495, YELLOW_TRANSPARENT);

	// Instructions
	vita2d_draw_texture(g_instructionBitmaps[3], 435, 510); // Left trigger button
	txtr_draw_text(463, 523, LIGHT_GREY, "Back");

	vita2d_end_drawing();
	vita2d_swap_buffers();
}

bool About::isExit(int buttons)
{
	if (buttons == SCE_CTRL_LTRIGGER || buttons == SCE_CTRL_LEFT)
		return true;

	return false;
}



