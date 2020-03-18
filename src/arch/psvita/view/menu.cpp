
/* menu.cpp: Main menu window.

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

#include "menu.h"
#include "resources.h"
#include "app_defs.h"
#include "debug_psv.h"
#include <vita2d.h>
#include <psp2/ctrl.h>


static int gs_mainMenuEntriesSize = 7;
static const char* gs_mainMenuEntries[] = 
{
	"Start game", 
	"Load/Save",
	"Controls", 
	"Settings",
	"Devices",
	"Reset", 
	"About"
};


MainMenu::MainMenu()
{
	m_imgMenu = NULL;
	m_imgMenuArrow = NULL;
	m_highlight = 0;
}

MainMenu::~MainMenu()
{
	if (m_imgMenu)
		vita2d_free_texture(m_imgMenu);
	if (m_imgMenuArrow)
		vita2d_free_texture(m_imgMenuArrow);
}

void MainMenu::init(View* view)
{
	m_view = view;
	loadResources();
}

void MainMenu::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	renderMenu();

	vita2d_end_drawing();
	vita2d_wait_rendering_done();
	vita2d_swap_buffers();
}

void MainMenu::render()
{
	renderMenu();
}

void MainMenu::doModal()
{
	m_selected = false;
	show();
	scanCyclic();
}

void MainMenu::navigateUp()
{
	if (m_highlight > 0){
		m_highlight--;
		show();
	}
}

void MainMenu::navigateDown()
{
	if (m_highlight < gs_mainMenuEntriesSize-1){
		m_highlight++;
		show();
	}
}

void MainMenu::buttonReleased(int button)
{
	if (isExit(button))
		Navigator::m_running = false;
}

bool MainMenu::isExit(int buttons)
{
	if(buttons == SCE_CTRL_CROSS || buttons == SCE_CTRL_RIGHT){ // Menu selection
		m_selected = true;
		m_selection = gs_mainMenuEntries[m_highlight];
		return true;
	}
	else if(buttons == SCE_CTRL_CIRCLE){ // Resume game
		m_selected = true;
		m_selection = "Resume game";
		return true;
	}
	
	return false;
}

string	MainMenu::getSelection()
{
	string ret;

	if (m_selected)
		ret = m_selection;

	return ret;
}

void MainMenu::renderMenu()
{
	// Menu
	vita2d_draw_texture(m_imgMenu, 305, 125);

	// Arrow
	vita2d_draw_texture(m_imgMenuArrow, 390, 185 + m_highlight*32);
}

void MainMenu::loadResources()
{
	m_imgMenu = vita2d_load_PNG_buffer(img_main_menu);
	m_imgMenuArrow = vita2d_load_PNG_buffer(img_main_menu_arrow);
}



