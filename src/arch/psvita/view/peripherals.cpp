
/* peripherals.cpp: Peripheral settings and control class.

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

#include "peripherals.h"
#include "controller.h"
#include "view.h"
#include "file_explorer.h"
#include "texter.h"
#include "ini_parser.h"
#include "guitools.h"
#include "resources.h"
#include "app_defs.h"
#include "debug_psv.h"
#include <cstring>
#include <vita2d.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h> 


// Max rows on the screen
#define MAX_ENTRIES 18
#define FONT_Y_SPACE 22
#define SCROLL_BAR_X 930
#define SCROLL_BAR_Y 35 
#define SCROLL_BAR_WIDTH 8
#define SCROLL_BAR_HEIGHT 450 


// Pointer to member function type
typedef void (Peripherals::*handler_func_t)(int, const char*);

typedef struct 
{
	string			display_name;
	string			ini_name;
	string			value;
	string			value2;
	const char**	values;
	int				values_size;
	int				isHeader;
	int				type;
	int				id;
	handler_func_t	handler; // Handler function when settings change
} PeripheralEntry;

static const char* gs_driveEmulationValues[]	      = {"Fast","True"};
static const char* gs_driveSoundEmulationValues[]     = {"Enabled","Disabled"};
static const char* gs_datasetteControlValues[]	      = {"Stop","Play","Forward","Rewind","Record","Reset","Reset counter"};
static const char* gs_datasetteCounterValues[]	      = {"Show","Hide"};
static const char* gs_datasetteResetWithCPUValues[]	  = {"Enabled","Disabled"};
static const char* gs_cartResetOnChangeValues[]	      = {"Enabled","Disabled"};


static int gs_peripheralEntriesSize = 12;
static PeripheralEntry gs_list[] = 
{
	{"Drive 8","","","",0,0,1}, /* Header line */
	{"Content","Drive8",             "Empty","",0,0,0,ST_MODEL,DRIVE8,0},
	{"Mode",   "DriveTrueEmulation", "Fast","",gs_driveEmulationValues,2,0,ST_MODEL,DRIVE_TRUE_EMULATION,0},
	{"Sound",  "DriveSoundEmulation","Disabled","",gs_driveSoundEmulationValues,2,0,ST_MODEL,DRIVE_SOUND_EMULATION,0},
	{"Datasette","","","",0,0,1},
	{"Content",       "Datasette",            "Empty","",0,0,0,ST_MODEL,DATASETTE,0},
	{"Control",       "DatasetteControl",     "Stop","",gs_datasetteControlValues,7,0,ST_MODEL,DATASETTE_CONTROL,0},
	{"Counter",       "DatasetteCounter",     "Hide","",gs_datasetteCounterValues,2,0,ST_VIEW,DATASETTE_COUNTER,0},
	{"Reset with CPU","DatasetteResetWithCPU","Enabled","",gs_datasetteResetWithCPUValues,2,0,ST_MODEL,DATASETTE_RESET_WITH_CPU,0},
	{"Cartridge","","","",0,0,1},
	{"Content",        "Cartridge",     "Empty","",0,0,0,ST_MODEL,CARTRIDGE,0},
	{"Reset on change","CartridgeReset","Enabled","",gs_cartResetOnChangeValues,2,0,ST_MODEL,CARTRIDGE_RESET,0},
};


Peripherals::Peripherals()
{
}

Peripherals::~Peripherals()
{
}

void Peripherals::init(View* view, Controller* controller)
{
	m_view = view;
	m_controller = controller;
	m_highlight = 1;
	m_borderTop = 0;
	m_borderBottom = MAX_ENTRIES-1;
	m_posXValue = 280;
	m_maxValueWidth = 850 - m_posXValue;
	m_selectingValue = false;

	// Set function pointers for the handlers
	for (int i=0; i<gs_peripheralEntriesSize; i++){
		
		if (gs_list[i].isHeader)
			continue;
		if (gs_list[i].type == ST_MODEL)
			gs_list[i].handler = &Peripherals::handleModelSetting;
		else if (gs_list[i].type == ST_VIEW)
			gs_list[i].handler = &Peripherals::handleViewSetting;
	}

	loadSettingsFromFile(DEF_CONF_FILE_PATH);
	
	m_scrollBar.init(SCROLL_BAR_X, SCROLL_BAR_Y, SCROLL_BAR_WIDTH, SCROLL_BAR_HEIGHT);
	m_scrollBar.setListSize(gs_peripheralEntriesSize, MAX_ENTRIES);
	m_scrollBar.setBackColor(GREY);
	m_scrollBar.setBarColor(ROYAL_BLUE);
}

RetCode Peripherals::doModal()
{
	m_exitCode = EXIT;
	m_controller->syncPeripherals();
	show();
	scanCyclic();	

	return m_exitCode;
}

void Peripherals::buttonReleased(int button)
{
	switch (button){
	case SCE_CTRL_SQUARE:
		// Save
		if (!isActionAllowed(PERIF_ACTION_SAVE))
			return;

		gtShowMsgBoxNoBtn("Saving...", this);
		sceKernelDelayThread(850000);
		saveSettingsToFile(DEF_CONF_FILE_PATH);
		m_settingsChanged = false;
		show();
		break;
	case SCE_CTRL_TRIANGLE:
		// Detach image
		if (!isActionAllowed(PERIF_ACTION_DETACH))
			return;

		detachImage(gs_list[m_highlight].id);
		show();
		break;
	case SCE_CTRL_CIRCLE: 
		// Attach image.
		{
			if (!isActionAllowed(PERIF_ACTION_ATTACH))
				return;

			string image = showFileBrowser(gs_list[m_highlight].id);
			
			if (!image.empty()){
				gtShowMsgBoxNoBtn("Attaching...");
				attachImage(gs_list[m_highlight].id, image.c_str());
			}

			show();
			break;
		}
	case SCE_CTRL_CROSS:
		// Load image
		{
			if (!isActionAllowed(PERIF_ACTION_LOAD))
				return;

			PeripheralEntry* entry = &gs_list[m_highlight];

			int ret = 0;
			if (entry->id == CARTRIDGE){
				ret = m_controller->loadFile(CART_LOAD, entry->value2.c_str());
			}
			else if (entry->id == DRIVE8){
				int index = getValueIndex(entry->value.c_str(), entry->values, entry->values_size);
				ret = m_controller->loadFile(DISK_LOAD, entry->value2.c_str(), entry->value.c_str(), index+1, entry->value.c_str());
			}
			else if (entry->id == DATASETTE){
				int index = getValueIndex(entry->value.c_str(), entry->values, entry->values_size);
				ret = m_controller->loadFile(TAPE_LOAD, entry->value2.c_str(), entry->value.c_str(), index, entry->value.c_str());
			}

			if (ret < 0){
				gtShowMsgBoxOk("Could not start program");
				show();
				break;
			}

			Navigator::m_running = false; 
			m_exitCode = EXIT_MENU; // Return to the game
		}
		break;
	case SCE_CTRL_RTRIGGER:
		// Freeze cartridge
		if (!isActionAllowed(PERIF_ACTION_FREEZE))
			return;

		m_controller->setCartControl(CART_CONTROL_FREEZE);
		break;
	}
}

bool Peripherals::isExit(int buttons)
{
	if (buttons == SCE_CTRL_LTRIGGER || buttons == SCE_CTRL_LEFT){ // Previous menu
		return true;
	}

	return false;
}

void Peripherals::navigateUp()
{
	if (m_highlight > 0){
		if (m_highlight-- == m_borderTop){
			m_borderTop--;
			m_borderBottom--;
			m_scrollBar.scrollUp();
		}

		// Don't highlight header
		for (int i=m_highlight; i>0; i--){
			if (!gs_list[i].isHeader) break;
			m_highlight--;
			if (m_highlight < m_borderTop){
				m_borderBottom--;
				m_borderTop--;
				m_scrollBar.scrollUp();
			}
		}

		// Here again we avoid the header item
		if (m_highlight == 0)
			m_highlight = 1;

		show();
	}
}

void Peripherals::navigateDown()
{
	// Move the highlight rectangle index down.
	// If index is out of screen, move the top and bottom boundaries down.
	// Make sure not to highlight the header items (Video, Input etc.)
	if (m_highlight < (gs_peripheralEntriesSize-1)){
		if (m_highlight++ == m_borderBottom){
			m_borderBottom++;
			m_borderTop++;
			m_scrollBar.scrollDown();
		}

		// Move the highlight index more down in case it's on a header
		for (int i=m_highlight; i<gs_peripheralEntriesSize-1; i++){
			if (!gs_list[i].isHeader)
				break;
			m_highlight++;
			if (m_highlight > m_borderBottom){
				m_borderBottom++;
				m_borderTop++;
				m_scrollBar.scrollDown();
			}
		}
		show();
	}
}

void Peripherals::navigateRight()
{
	// Change setting value.

	if (gs_list[m_highlight].values_size < 2) // No values to choose
		return;

	m_selectingValue = true;
	string selection = showValuesListBox(gs_list[m_highlight].values, gs_list[m_highlight].values_size);
				
	if (!selection.empty()){
		if (gs_list[m_highlight].value != selection){
			gs_list[m_highlight].value = selection;
			(this->*gs_list[m_highlight].handler)(gs_list[m_highlight].id, gs_list[m_highlight].value.c_str());
			if (!naviOnPeripherals())
				m_settingsChanged = true;
		}
	}

	m_selectingValue = false;
	show();
}

void Peripherals::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing();
	vita2d_swap_buffers();
	vita2d_wait_rendering_done();
}

void Peripherals::render()
{ 
	int y = 55;
	int key_color, val_color, arr_color;
	int start = m_borderTop;
	int end = (gs_peripheralEntriesSize > MAX_ENTRIES)? m_borderBottom: gs_peripheralEntriesSize-1;

	// Top seperation line
	vita2d_draw_line(15, 25, 940, 25, YELLOW_TRANSPARENT);

	for (int i=start; i<=end; ++i){
		PeripheralEntry* entry = &gs_list[i];
		// Add space between header sections, skip topmost.
		if (entry->isHeader && i != start)
			y += 5;

		if (entry->isHeader){
			txtr_draw_text(20, y, WHITE, entry->display_name.c_str());
			// Draw line under header
			y += 4;
			vita2d_draw_line(20, y, 900, y, WHITE);
		}
		else{
			key_color = arr_color = YELLOW;
			val_color = YELLOW;

			// Highlight rectangle
			if (i == m_highlight){
				int textHeight = txtr_get_text_height(entry->display_name.c_str(), 24);
				// Draw highlight rectangle
				vita2d_draw_rectangle(35, y-textHeight+1, 870, textHeight+2, ROYAL_BLUE);
				m_highligtBarYpos = y-textHeight+2; // save this for listbox position
				key_color = val_color = arr_color = WHITE;
			}	
			// Key
			txtr_draw_text(40, y, key_color, entry->display_name.c_str());
			// Value
			txtr_draw_text(m_posXValue, y, val_color, getDisplayFitString(entry->value.c_str(), m_maxValueWidth).c_str());
			// Draw left arrow if key is highlighted
			if (i == m_highlight && entry->values_size > 1){
				int arrow_pos_x = m_posXValue;
				arrow_pos_x += txtr_get_text_width(getDisplayFitString(entry->value.c_str(), m_maxValueWidth).c_str(), 24) + 15;
				txtr_draw_text(arrow_pos_x, y, arr_color, ">");
			}
		}

		y += FONT_Y_SPACE;
	}

	// Scroll bar
	if (gs_peripheralEntriesSize > MAX_ENTRIES)
		m_scrollBar.render();

	// Bottom seperation line
	vita2d_draw_line(15, 495, 940, 495, YELLOW_TRANSPARENT);
	// Instructions
	renderInstructions();
}

void Peripherals::renderInstructions()
{
	if (m_selectingValue){
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_X], 400, 510); // Navigate buttons
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 495, 510); // Dpad left button
		txtr_draw_text(521, 523, LIGHT_GREY, "Back");
		return;
	}

	int offset_x=0;
	if (naviOnPeripherals()){
		if (gs_list[m_highlight].value == "Empty"){
			if (m_settingsChanged)
				offset_x = -60;
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN], offset_x+=357, 510); 
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], offset_x+=65, 510);
			txtr_draw_text(offset_x+=22, 523, LIGHT_GREY, "Attach");
			offset_x+=93;
		}
		else{
			if (gs_list[m_highlight].id == CARTRIDGE){
				if (m_settingsChanged)
					offset_x = -60;
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN], offset_x+=160, 510);
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], offset_x+=70, 510);
				txtr_draw_text(offset_x+=22, 523, LIGHT_GREY, "Attach");
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_TRIANGLE_BLUE], offset_x+=88, 511);
				txtr_draw_text(offset_x+=33, 523, LIGHT_GREY, "Detach");
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CROSS_BLUE], offset_x+=92, 510);
				txtr_draw_text(offset_x+=20, 523, LIGHT_GREY, "Auto load");
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_RTRIGGER_BLUE], offset_x+=120, 508);
				txtr_draw_text(offset_x+=40, 523, LIGHT_GREY, "Freeze");
				offset_x+=95;
			}
			else{
				if (m_settingsChanged)
					offset_x = -60;
				if (gs_list[m_highlight].values_size > 1)
					vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], offset_x+=210, 510);
				else{
					vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN], offset_x+=225, 510);
					offset_x-=28;
				}
				
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], offset_x+=90, 510);
				txtr_draw_text(offset_x+=22, 523, LIGHT_GREY, "Attach");
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_TRIANGLE_BLUE], offset_x+=88, 511);
				txtr_draw_text(offset_x+=33, 523, LIGHT_GREY, "Detach");
				vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CROSS_BLUE], offset_x+=92, 510);
				txtr_draw_text(offset_x+=20, 523, LIGHT_GREY, "Auto load");
				offset_x+=120;
			}
		}
	}
	else{
		if (m_settingsChanged)
			offset_x = -60;
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], offset_x+=400, 510);
		offset_x+=95;
	}

	vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], offset_x, 510);
	txtr_draw_text(offset_x+=26, 523, LIGHT_GREY, "Exit");
	
	if (m_settingsChanged){
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], offset_x+=70, 510);
		txtr_draw_text(offset_x+25, 523, LIGHT_GREY, "Save");
	}	
}

bool Peripherals::isActionAllowed(PeripheralsAction action)
{
	switch (action)
	{
	case PERIF_ACTION_SAVE:
		return m_settingsChanged? true: false;
	case PERIF_ACTION_ATTACH:
	case PERIF_ACTION_DETACH:
		return naviOnPeripherals()? true: false;
	case PERIF_ACTION_LOAD:
		return (naviOnPeripherals() && gs_list[m_highlight].value != "Empty")? true: false;
	case PERIF_ACTION_FREEZE:
		return (gs_list[m_highlight].id == CARTRIDGE && gs_list[m_highlight].value != "Empty")? true: false;
	default:
		return false;
	};
}

void Peripherals::loadSettingsFromFile(const char* ini_file)
{
	IniParser ini_parser;
	ini_parser.init(ini_file);

	char key_value[128];

	for (int i=0; i<gs_peripheralEntriesSize; i++){

		if (gs_list[i].isHeader)
			continue;

		memset(key_value, 0, 128);
		if (!ini_parser.getKeyValue(ini_file, "Peripherals", gs_list[i].ini_name.c_str(), key_value) && strlen(key_value) != 0)
			gs_list[i].value = key_value;
	}
}

string Peripherals::showValuesListBox(const char** values, int size)
{
	// Calculate x coordinate, y is already available.
	int x = m_posXValue + txtr_get_text_width(gs_list[m_highlight].value.c_str(), 24) + 35;

	return gtShowListBox(x, m_highligtBarYpos-1, 0, 0, values, size, this);
}

string Peripherals::showFileBrowser(int peripheral)
{
	// Remember last visited folder and navigation spot.
	static string last_game_dir = GAME_DIR;
	static int last_highlight_index = 0;
	static int last_bordertop_index = 0;
	static float last_scrollbar_ypos = 0;

	static const char* filter[] = {
	   "CRT",														// Cartridge image
       "D64","D71","D80","D81","D82","G64","G41","X64",				// Disk image
       "T64","TAP",													// Tape image
	   "PRG","P00",													// Program image
	   "ZIP",														// Archive file
	   NULL};					

	FileExplorer fileExp;
	fileExp.init(last_game_dir.c_str(), 
				last_highlight_index, 
				last_bordertop_index, 
				last_scrollbar_ypos,
				filter);

	string selection = fileExp.doModal();
	
	last_game_dir = fileExp.getDir();
	last_highlight_index = fileExp.getHighlightIndex();
	last_bordertop_index = fileExp.getBorderTopIndex();
	last_scrollbar_ypos = fileExp.getScrollBarPosY();

	return selection;
}

void Peripherals::saveSettingsToFile(const char* ini_file)
{
	IniParser ini_parser;
	int res = ini_parser.init(ini_file);

	if (res != INI_PARSER_OK)
		return;

	for (int i = 0; i<gs_peripheralEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		ini_parser.setKeyValue(ini_file, "Peripherals", gs_list[i].ini_name.c_str(), gs_list[i].value.c_str());	

	}

	ini_parser.saveToFile(ini_file);
}

string Peripherals::getKeyValue(int key)
{
	string ret;

	for (int i = 0; i<gs_peripheralEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].id == key)
			ret = gs_list[i].value;
	}

	return ret;
}

void Peripherals::setKeyValue(int key, const char* value, const char* value2, const char** values, int size, int mask)
{
	for (int i = 0; i<gs_peripheralEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].id == key){
			if (mask & 0x01)
				gs_list[i].value = value;
			if (mask & 0x02)
				gs_list[i].value2 = value2;
			if (mask & 0x04)
				gs_list[i].values = values;
			if (mask & 0x08)
				gs_list[i].values_size = size;
		}
	}
}

void Peripherals::getKeyValues(int key, const char** value, const char** value2, const char*** values, int* size)
{
	for (int i = 0; i<gs_peripheralEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].id == key){
			if (value)
				*value = gs_list[i].value.c_str();
			if (value2)
				*value2 = gs_list[i].value2.c_str();
			if (values)
				*values = gs_list[i].values;
			if (size)
				*size = gs_list[i].values_size;
		}
	}
}


void Peripherals::applyAllSettings()
{
	for (int i = 0; i<gs_peripheralEntriesSize; ++i){
		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].handler)
			(this->*gs_list[i].handler)(gs_list[i].id, gs_list[i].value.c_str());
	}
}
	
void Peripherals::handleModelSetting(int key, const char* value)
{
	switch (key){
	case DATASETTE_CONTROL:
		{
		int cmd = 0;
		if (!strcmp(value, "Stop"))
			cmd = TAPE_CONTROL_STOP;
		else if (!strcmp(value, "Play"))
			cmd = TAPE_CONTROL_PLAY;
		else if (!strcmp(value, "Forward"))
			cmd = TAPE_CONTROL_FORWARD;
		else if (!strcmp(value, "Rewind"))
			cmd = TAPE_CONTROL_REWIND;
		else if (!strcmp(value, "Record"))
			cmd = TAPE_CONTROL_RECORD;
		else if (!strcmp(value, "Reset"))
			cmd = TAPE_CONTROL_RESET;
		else if (!strcmp(value, "Reset counter"))
			cmd = TAPE_CONTROL_RESET_COUNTER;

		m_controller->setTapeControl(cmd);
		break;
		}
	case DRIVE_TRUE_EMULATION:
	case DRIVE_SOUND_EMULATION:
	case DATASETTE_RESET_WITH_CPU:
	case CARTRIDGE_RESET:
		m_controller->setModelProperty(key, value);
		break;
	}
}

void Peripherals::handleViewSetting(int key, const char* value)
{
	m_view->setProperty(key, value);
}

bool Peripherals::naviOnPeripherals(){
	if (gs_list[m_highlight].id == DRIVE8 ||
	    gs_list[m_highlight].id == DATASETTE ||
	    gs_list[m_highlight].id == CARTRIDGE)
	    return true;
		
	return false;
}

string	Peripherals::getDisplayFitString(const char* str, int limit, float font_size)
{
	// Returns a shrinked string that fits to the limit boundaries.
	string ret;

	if (!str || !limit){
		return ret;
	}

	ret = str;
	int str_width = txtr_get_text_width(str, font_size);

	if (str_width <= limit){
		return ret;
	}

	while(str_width > limit){
		ret.pop_back();
		str_width = txtr_get_text_width(ret.c_str(), font_size);
	}

	ret.append("...");

	return ret;
}

int Peripherals::getValueIndex(const char* value, const char** values, int size)
{
	for (int i=0; i<size; ++i)
	{
		if (!strcmp(value, values[i]))
			return i; 
	}

	return 0;
}

string Peripherals::getFileNameFromPath(const char* fpath)
{
	string fname = fpath;

	size_t slash_pos = fname.find_last_of("/");
	if (slash_pos != string::npos){
		return fname.substr(slash_pos+1, string::npos);
	}

	size_t colon_pos = fname.find_last_of(":");
	if (colon_pos != string::npos){
		return fname.substr(colon_pos+1, string::npos);
	}

	return fname;
}

void Peripherals::attachImage(int peripheral, const char* image)
{
	int index = getKeyIndex(peripheral);

	if (index < 0)
		return;
	
	gtShowMsgBoxNoBtn("Attaching...");
	if (m_controller->attachImage(peripheral, image, gs_list[index].values, gs_list[index].values_size) < 0)
		return;
}

void Peripherals::detachImage(int peripheral)
{
	int index = getKeyIndex(peripheral);
	
	if (index < 0)
		return;

	m_controller->detachImage(peripheral, gs_list[index].values, gs_list[index].values_size);
}

int	Peripherals::getKeyIndex(int key)
{
	for (int i = 0; i<gs_peripheralEntriesSize; ++i){
		if (gs_list[i].isHeader)
			continue;
		if(gs_list[i].id == key)
			return i;
	}
	return -1;
}


