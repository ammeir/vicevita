
/* settings.cpp: Program settings manager.

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

#include "settings.h"
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
#define SCROLL_BAR_Y 45 
#define SCROLL_BAR_WIDTH 8
#define SCROLL_BAR_HEIGHT 430 

// Pointer to member function type
typedef void (Settings::*handler_func_t)(int, const char*);

typedef struct 
{
	string			key_display_name;
	string			key_ini_name;
	string			value;
	const char**	values;
	int				values_size;
	string			data_src; // Store source file name here if values are dynamic.
	int				isHeader;
	int				type;
	int				id;
	handler_func_t	handler; // Handler function when settings change
} SettingsEntry;


static const char* gs_viciiModelValues[]		= {"PAL","NTSC","Old NTSC","PAL-N"};
static const char* gs_sidEngineValues[]			= {"FastSID","ReSID"};
static const char* gs_sidModelValues[]			= {"6581","8580"};
static const char* gs_aspectRatioValues[]		= {"16:9","4:3","4:3 max"};
static const char* gs_textureFilterValues[]		= {"Point","Linear"};
static const char* gs_colorPaletteValues[]		= {"Pepto (PAL)","Colodore","Vice","Ptoing","RGB","None"};
static const char* gs_borderVisibilityValues[]	= {"Show","Hide","Remove"};
static const char* gs_joystickPortValues[]		= {"Port 1","Port 2"};
static const char* gs_joystickSideValues[]		= {"Left","Right"};
static const char* gs_keyboardModeValues[]		= {"Full screen","Split screen","Slider"};
static const char* gs_autofireSpeedValues[]		= {"Slow","Medium","Fast"};
static const char* gs_cpuSpeedValues[]			= {"100%","125%","150%","175%","200%"};
static const char* gs_hostCpuSpeedValues[]		= {"333 MHz","444 MHz"};
static const char* gs_audioPlaybackValues[]		= {"Enabled","Disabled"};
static const char* gs_machineResetValues[]		= {"Hard","Soft"};

static int gs_settingsEntriesSize = 21;
static SettingsEntry gs_list[] = 
{
	{"Machine","","",0,0,"",1}, /* Header line */
	{"VIC-II model","VICIIModel","PAL",gs_viciiModelValues,4,"",0,ST_MODEL,VICII_MODEL,0},
	{"SID engine",  "SIDEngine", "FastSID",gs_sidEngineValues,2,"",0,ST_MODEL,SID_ENGINE,0},
	{"SID model",   "SIDModel",  "6581",gs_sidModelValues,2,"",0,ST_MODEL,SID_MODEL,0},
	{"Video","","",0,0,"",1},
	{"Aspect ratio",  "AspectRatio",  "16:9",gs_aspectRatioValues,3,"",0,ST_VIEW,ASPECT_RATIO,0},
	{"Texture filter","TextureFilter","Linear",gs_textureFilterValues,2,"",0,ST_VIEW,TEXTURE_FILTER,0},
	{"Color palette", "ColorPalette", "Colodore",gs_colorPaletteValues,6,"",0,ST_MODEL,COLOR_PALETTE,0},
	{"Borders",       "Borders",      "Hide",gs_borderVisibilityValues,2,"",0,ST_VIEW,BORDERS,0},
	{"Input","","",0,0,"",1},
	{"Joystick port", "JoystickPort", "Port 2",gs_joystickPortValues,2,"",0,ST_MODEL,JOYSTICK_PORT,0},
	{"Joystick side", "JoystickSide", "Left",gs_joystickSideValues,2,"",0,ST_VIEW,JOYSTICK_SIDE,0},
	{"Autofire speed","AutofireSpeed","Fast",gs_autofireSpeedValues,3,"",0,ST_VIEW,JOYSTICK_AUTOFIRE_SPEED,0},
	{"Keyboard mode", "KeyboardMode", "Slider",gs_keyboardModeValues,3,"",0,ST_VIEW,KEYBOARD_MODE,0},
	{"Performance","","",0,0,"",1},
	{"CPU speed",     "CPUSpeed",    "100%",gs_cpuSpeedValues,5,"",0,ST_MODEL,CPU_SPEED,0},
	{"Host CPU speed","HostCPUSpeed","333 MHz",gs_hostCpuSpeedValues,2,"",0,ST_VIEW,HOST_CPU_SPEED,0},
	{"Audio","","",0,0,"",1},
	{"Playback","Sound","Enabled",gs_audioPlaybackValues,2,"",0,ST_MODEL,SOUND,0},
	{"Other","","",0,0,"",1},
	{"Reset","Reset","Hard",gs_machineResetValues,2,"",0,ST_MODEL,MACHINE_RESET,0}
};



Settings::Settings()
{
	m_defSettings = NULL;
}

Settings::~Settings()
{
	if (m_defSettings){
		delete[] m_defSettings;
	}
}

void Settings::init(View* view, Controller* controller)
{
	m_view = view;
	m_controller = controller;
	m_highlight = 1;
	m_borderTop = 0;
	m_borderBottom = MAX_ENTRIES-1;
	m_posXValue = 280;
	m_maxValueWidth = 850 - m_posXValue;
	m_userChanges = false;

	// Set function pointers for the handlers.
	for (int i=0; i<gs_settingsEntriesSize; i++){
		
		if (gs_list[i].isHeader)
			continue;
		if (gs_list[i].type == ST_MODEL)
			gs_list[i].handler = &Settings::handleModelSetting;
		else if (gs_list[i].type == ST_VIEW)
			gs_list[i].handler = &Settings::handleViewSetting;
	}

	if (!settingsExistInFile(DEF_CONF_FILE_PATH)){
		// We enter here in following cases:
		// - Key values empty (first run of the app).
		// - Missing keys (new version update).
		saveSettingsToFile(DEF_CONF_FILE_PATH, false);
	}
	
	loadSettingsFromFile(DEF_CONF_FILE_PATH);
	loadDefSettingsArray();

	m_scrollBar.init(SCROLL_BAR_X, SCROLL_BAR_Y, SCROLL_BAR_WIDTH, SCROLL_BAR_HEIGHT);
	m_scrollBar.setListSize(gs_settingsEntriesSize, MAX_ENTRIES);
	m_scrollBar.setBackColor(GREY);
	m_scrollBar.setBarColor(ROYAL_BLUE);
}

RetCode Settings::doModal(const char* save_dir, const char* file_name)
{
	m_saveDir = save_dir;
	changeState();
	// Game file header name. Shrink the string to fit the screen.
	int max_width = 890 - txtr_get_text_width(m_confFileDesc.c_str(), 22);
	m_gameFileHeader = getDisplayFitString(file_name, max_width);
	m_exitCode = EXIT;
	show();
	scanCyclic();	

	return m_exitCode;
}

void Settings::buttonReleased(int button)
{
	switch (button){
	case SCE_CTRL_SQUARE:
		{
		// Save settings to game specific or default conf file
		if (!isActionAllowed(STN_ACTION_SAVE))
			return;

		string conf_file_path = m_saveDir.empty()? DEF_CONF_FILE_PATH: m_saveDir + CONF_FILE_NAME;
		
		// Create config.ini if it doesn't exist
		if (!fileExist(conf_file_path.c_str())){
			createConfFile(conf_file_path.c_str());
		}
		gtShowMsgBoxNoBtn("Saving...", this);
		sceKernelDelayThread(850000);
		saveSettingsToFile(conf_file_path.c_str());

		if (conf_file_path == DEF_CONF_FILE_PATH){
			loadDefSettingsArray();
		}
		m_userChanges = false;
		changeState();
		show();
		break;
		}
	case SCE_CTRL_CIRCLE:
		// Load default settings
		if (!isActionAllowed(STN_ACTION_LOAD_DEFAULT))
			return;
		loadSettingsFromFile(DEF_CONF_FILE_PATH);
		applySettings(SETTINGS_ALL);
		m_userChanges = true;
		changeState();
		show();
		break;
	};
}

bool Settings::isExit(int buttons)
{
	if (buttons == SCE_CTRL_LTRIGGER || buttons == SCE_CTRL_LEFT){ // Previous menu
		return true;
	}

	return false;
}

void Settings::navigateUp()
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

void Settings::navigateDown()
{
	// Move the highlight rectangle index down.
	// If index is out of screen, move the top and bottom boundaries down.
	// Make sure not to highlight the header items (Video, Input etc.)
	if (m_highlight < (gs_settingsEntriesSize-1)){
		if (m_highlight++ == m_borderBottom){
			m_borderBottom++;
			m_borderTop++;
			m_scrollBar.scrollDown();
		}

		// Move the highlight index more down in case it's on a header
		for (int i=m_highlight; i<gs_settingsEntriesSize-1; i++){
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

void Settings::navigateRight()
{
	// Change setting value.
	SettingsState prev_state = m_state;
	m_state = STN_STATE_SELECTING; // User selecting state
	string selection = showValuesListBox(gs_list[m_highlight].values, gs_list[m_highlight].values_size);
	m_state = prev_state;

	if (!selection.empty()){
		if (gs_list[m_highlight].value != selection){
			gs_list[m_highlight].value = selection;
			(this->*gs_list[m_highlight].handler)(gs_list[m_highlight].id, gs_list[m_highlight].value.c_str());
			m_userChanges = true;
			changeState();
		}
	}

	show();
}

void Settings::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing();
	vita2d_swap_buffers();
	vita2d_wait_rendering_done();
}

void Settings::render()
{ 
	int y = 60;
	
	// Game file name
	txtr_draw_text(15, 20, C64_BLUE, m_gameFileHeader.c_str());
	// Conf file type
	txtr_draw_text(855, 20, C64_BLUE, m_confFileDesc.c_str());
	// Top seperation line
	vita2d_draw_line(15, 30, 940, 30, YELLOW_TRANSPARENT);

	int start = m_borderTop;
	int key_color, val_color, arr_color;
	int end = (gs_settingsEntriesSize > MAX_ENTRIES)? m_borderBottom: gs_settingsEntriesSize-1;
	
	for (int i=start; i<=end; ++i){
		// Add space between header sections, skip topmost.
		if (gs_list[i].isHeader && i != start)
			y += 5;

		if (gs_list[i].isHeader){
			txtr_draw_text(20, y, WHITE, gs_list[i].key_display_name.c_str());
			// Draw line under header
			y += 4;
			vita2d_draw_line(20, y, 900, y, WHITE);
		}
		else{
			key_color = arr_color = YELLOW;
			val_color = YELLOW;

			// Highlight rectangle
			if (i == m_highlight){
				int textHeight = txtr_get_text_height(gs_list[i].key_display_name.c_str(), 24);
				// Draw highlight rectangle
				vita2d_draw_rectangle(35, y-textHeight+1, 870, textHeight+2, ROYAL_BLUE);
				m_highligtBarYpos = y-textHeight+2; // save this for listbox position
				key_color = val_color = arr_color = WHITE;
			}	
			// Key
			txtr_draw_text(40, y, key_color, gs_list[i].key_display_name.c_str());
			// Value
			txtr_draw_text(m_posXValue, y, val_color, getDisplayFitString(gs_list[i].value.c_str(), m_maxValueWidth).c_str());
			// Draw left arrow if key is highlighted
			if (i == m_highlight && gs_list[i].values_size > 1){
				int arrow_pos_x = m_posXValue;
				arrow_pos_x += txtr_get_text_width(getDisplayFitString(gs_list[i].value.c_str(), m_maxValueWidth).c_str(), 24) + 15;
				txtr_draw_text(arrow_pos_x, y, arr_color, ">");
			}
		}

		y += FONT_Y_SPACE;
	}

	// Scroll bar
	if (gs_settingsEntriesSize > MAX_ENTRIES)
		m_scrollBar.render();

	// Bottom seperation line
	vita2d_draw_line(15, 495, 940, 495, YELLOW_TRANSPARENT);
	// Instructions
	renderInstructions();
}

void Settings::renderInstructions()
{
	switch (m_state){

	case STN_STATE_DEFAULT_CONF:
		// Default controls shown. Game not loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 395, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 490, 510);
		txtr_draw_text(516, 523, LIGHT_GREY, "Exit");
		break;
	case STN_STATE_INGAME_DEFAULT_CONF:
		// Default controls shown. Game loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 325, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 420, 510);
		txtr_draw_text(446, 523, LIGHT_GREY, "Exit");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 506, 510);
		txtr_draw_text(531, 523, LIGHT_GREY, "Save");
		break;
	case STN_STATE_SELECTING:
		// User selecting new value
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_X], 395, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 490, 510);
		txtr_draw_text(516, 523, LIGHT_GREY, "Back");
		break;
	case STN_STATE_GAME_CONF:
		// Game opened. Customized controls shown.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 300, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 390, 510);
		txtr_draw_text(413, 523, LIGHT_GREY, "Load default");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 560, 510);
		txtr_draw_text(585, 523, LIGHT_GREY, "Exit");
		break;
	case STN_STATE_DEFAULT_MOD: // Controls modified. Game not loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 200, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 290, 510);
		txtr_draw_text(313, 523, LIGHT_GREY, "Load default");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 460, 510);
		txtr_draw_text(485, 523, LIGHT_GREY, "Exit");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 548, 510);
		txtr_draw_text(573, 523, LIGHT_GREY, "Save as default");
		break;
	case STN_STATE_INGAME_MOD: // Controls modified. Game loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 270, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 360, 510);
		txtr_draw_text(383, 523, LIGHT_GREY, "Load default");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 530, 510);
		txtr_draw_text(555, 523, LIGHT_GREY, "Exit");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 615, 510);
		txtr_draw_text(640, 523, LIGHT_GREY, "Save");
		break;
	};
}

bool Settings::isActionAllowed(SettingsAction action)
{
	switch (action)
	{
	case STN_ACTION_SAVE:
		return (m_state == STN_STATE_INGAME_DEFAULT_CONF ||
				m_state == STN_STATE_DEFAULT_MOD ||
				m_state == STN_STATE_INGAME_MOD)? true: false;
	case STN_ACTION_LOAD_DEFAULT:
		return (m_state == STN_STATE_GAME_CONF || 
				m_state == STN_STATE_DEFAULT_MOD ||
				m_state == STN_STATE_INGAME_MOD)? true: false;
	default:
		return false;
	};
}

void Settings::loadSettingsFromFile(const char* ini_file)
{
	IniParser ini_parser;

	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return;

	char key_value[128];

	for (int i=0; i<gs_settingsEntriesSize; i++){

		if (gs_list[i].isHeader)
			continue;

		memset(key_value, 0, 128);
		if (ini_parser.getKeyValue(INI_FILE_SEC_SETTINGS, gs_list[i].key_ini_name.c_str(), key_value) == INI_PARSER_KEY_NOT_FOUND) 
			continue;
			
		if (strlen(key_value) == 0)
			continue;

		gs_list[i].value = key_value;
		
		if (gs_list[i].id == CPU_SPEED) // % is not in ini file (HACK).
			gs_list[i].value.append("%");
	}
}

string Settings::showValuesListBox(const char** values, int size)
{
	// Calculate x coordinate, y is already available.
	int x = m_posXValue + txtr_get_text_width(gs_list[m_highlight].value.c_str(), 24) + 30;
	return gtShowListBox(x, m_highligtBarYpos-1, 0, 0, values, size, this, gs_list[m_highlight].value.c_str());
}

void Settings::saveSettingsToFile(const char* ini_file, bool over_write)
{
	IniParser ini_parser;

	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return;

	char key_value[128] = {0};

	for (int i = 0; i<gs_settingsEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		int ret = ini_parser.getKeyValue(INI_FILE_SEC_SETTINGS, gs_list[i].key_ini_name.c_str(), key_value);

		if (ret == INI_PARSER_KEY_NOT_FOUND){
			// Old ini file version. Add new key/value pair.
			ini_parser.addKeyToSec(INI_FILE_SEC_SETTINGS, gs_list[i].key_ini_name.c_str(), gs_list[i].value.c_str());
			continue;
		}

		if (strlen(key_value) == 0){
			// Key exist without value.
			ini_parser.setKeyValue(INI_FILE_SEC_SETTINGS, gs_list[i].key_ini_name.c_str(), gs_list[i].value.c_str());	
			continue;
		}
		
		// Value exist. 
		if (over_write){
			ini_parser.setKeyValue(INI_FILE_SEC_SETTINGS, gs_list[i].key_ini_name.c_str(), gs_list[i].value.c_str());	
		}
	}

	ini_parser.saveToFile(ini_file);
}

string Settings::getKeyValue(int key)
{
	string ret;

	for (int i = 0; i<gs_settingsEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].id == key)
			ret = gs_list[i].value;
	}

	return ret;
}

void Settings::setKeyValue(int key, const char* value, const char* src, const char** values, int size, int mask)
{
	for (int i = 0; i<gs_settingsEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].id == key){
			if (mask & 0x01)
				gs_list[i].value = value;
			if (mask & 0x02)
				gs_list[i].data_src = src;
			if (mask & 0x04)
				gs_list[i].values = values;
			if (mask & 0x08)
				gs_list[i].values_size = size;
		}
	}
}

void Settings::setKeyValue(const char* key, const char* value, const char* src, const char** values, int size, int mask)
{
	for (int i = 0; i<gs_settingsEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].key_ini_name == key){
			if (mask & 0x01)
				gs_list[i].value = value;
			if (mask & 0x02)
				gs_list[i].data_src = src;
			if (mask & 0x04)
				gs_list[i].values = values;
			if (mask & 0x08)
				gs_list[i].values_size = size;
		}
	}
}

void Settings::getKeyValues(int key, const char** value, const char** src, const char*** values, int* size)
{
	for (int i = 0; i<gs_settingsEntriesSize; ++i){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].id == key){
			if (value)
				*value = gs_list[i].value.c_str();
			if (src)
				*src = gs_list[i].data_src.c_str();
			if (values)
				*values = gs_list[i].values;
			if (size)
				*size = gs_list[i].values_size;
		}
	}
}

void Settings::applySetting(int key)
{
	for (int i = 0; i<gs_settingsEntriesSize; ++i){
		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].id == key && gs_list[i].handler){
			(this->*gs_list[i].handler)(gs_list[i].id, gs_list[i].value.c_str());
			break;
		}
	}
}

void Settings::applySettings(int group)
{
	switch (group){
	case SETTINGS_ALL:
		for (int i = 0; i<gs_settingsEntriesSize; ++i){
			if (gs_list[i].isHeader)
				continue;

			if (gs_list[i].handler)
				(this->*gs_list[i].handler)(gs_list[i].id, gs_list[i].value.c_str());
		}
		break;
	case SETTINGS_VIEW:
		for (int i = 0; i<gs_settingsEntriesSize; ++i){
			if (gs_list[i].isHeader)
				continue;

			if (gs_list[i].type == ST_VIEW && gs_list[i].handler)
				(this->*gs_list[i].handler)(gs_list[i].id, gs_list[i].value.c_str());
		}
		break;
	case SETTINGS_MODEL:
		for (int i = 0; i<gs_settingsEntriesSize; ++i){
			if (gs_list[i].isHeader)
				continue;

			if (gs_list[i].type == ST_MODEL && gs_list[i].handler)
				(this->*gs_list[i].handler)(gs_list[i].id, gs_list[i].value.c_str());
		}
		break;
	case SETTINGS_MODEL_NOT_IN_SNAP:
		applySetting(COLOR_PALETTE);
		applySetting(CPU_SPEED);
		applySetting(SID_MODEL);
		break;
	}
}

void Settings::createConfFile(const char* file)
{
	// Create conficuration file.

	if (!file)
		return;

	FileExplorer fileExp;
	string conf_dir = getDirOfFile(file);

	if (conf_dir.empty())
		return;

	if (!fileExp.dirExist(conf_dir.c_str()))
		fileExp.makeDir(conf_dir.c_str());

	if (!fileExp.fileExist(file)){
		char buf [1024] = {0};

		strcpy(buf, "[Controls]");
		strcat(buf, "\x0D\x0A"); // new line
		strcat(buf, "Keymaps="); 
		strcat(buf, "\x0D\x0A");
		strcat(buf, "[Settings]");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "VICIIModel=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "SIDEngine=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "SIDModel=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "AspectRatio=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "TextureFilter=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "ColorPalette=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "Borders=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "JoystickPort=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "JoystickSide=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "AutofireSpeed=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "KeyboardMode=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "CPUSpeed=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "HostCPUSpeed=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "Sound=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "Reset=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "[Peripherals]");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "DriveTrueEmulation=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "DriveSoundEmulation=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "DatasetteResetWithCPU=");
		strcat(buf, "\x0D\x0A");
		strcat(buf, "CartridgeReset=");
		strcat(buf, "\x0D\x0A");
		
		fileExp.writeToFile(file, buf, strlen(buf));
	}
}

string Settings::toString(int setting)
{
	// Format: key1^value|key2^value|...|keyn^value

	string ret;

	switch (setting){
	case SETTINGS_VIEW:
		// Settings that are related to the View.
		for (int i = 0; i<gs_settingsEntriesSize; ++i){
			if (gs_list[i].isHeader)
				continue;

			switch (gs_list[i].id){
			case ASPECT_RATIO:
			case TEXTURE_FILTER:
			case BORDERS:
			case JOYSTICK_SIDE:
			case JOYSTICK_AUTOFIRE_SPEED:
			case KEYBOARD_MODE:
			case HOST_CPU_SPEED:
				ret.append(gs_list[i].key_ini_name);
				ret.append(SNAP_MOD_DELIM_FIELD);
				ret.append(gs_list[i].value);
				ret.append(SNAP_MOD_DELIM_ENTRY);
			}
		}
		break;
	case SETTINGS_MODEL:
		// Settings that are related to the Model.
		break;
	case SETTINGS_MODEL_NOT_IN_SNAP:
		// Model settings that are not saved in the snapshot.
		for (int i = 0; i<gs_settingsEntriesSize; ++i){
			if (gs_list[i].isHeader)
				continue;

			switch (gs_list[i].id){
			case COLOR_PALETTE:
			case CPU_SPEED:
			case SID_MODEL:
				ret.append(gs_list[i].key_ini_name);
				ret.append(SNAP_MOD_DELIM_FIELD);
				ret.append(gs_list[i].value);
				ret.append(SNAP_MOD_DELIM_ENTRY);
			}
		}
		break;
	}

	if (!ret.empty())
		ret.pop_back(); // Remove last delimeter

	return ret;
}
	
void Settings::handleModelSetting(int key, const char* value)
{
	m_controller->setModelProperty(key, value);
}

void Settings::handleViewSetting(int key, const char* value)
{
	m_view->setProperty(key, value);
}

string	Settings::getDisplayFitString(const char* str, int limit, float font_size)
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

bool Settings::fileExist(const char* file_name) 
 {
    if (FILE *file = fopen(file_name, "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

string Settings::getDirOfFile(const char* file)
{
	string str = file;
	size_t pos = str.find_last_of("/");
	if (pos != string::npos)
		str = str.substr(0, pos);

	return str;
}

void Settings::changeState()
{
	int diff = cmpSettingsToDef();

	if (m_saveDir.empty()){ 
		// This is a case when the app was just started and no game/basic is yet loaded.
		m_state = (diff)? STN_STATE_DEFAULT_MOD: STN_STATE_DEFAULT_CONF;
		m_confFileDesc = "[Default]";
		return;
	}

	// Game or Basic loaded.
	if (diff){
		m_state = m_userChanges? STN_STATE_INGAME_MOD: STN_STATE_GAME_CONF;
		m_confFileDesc = "[Custom]";
	}else{
		m_state = m_userChanges? STN_STATE_INGAME_DEFAULT_CONF: STN_STATE_DEFAULT_CONF;
		m_confFileDesc = "[Default]";
	}
}

bool Settings::settingsExistInFile(const char* ini_file)
{
	// Returns true if all current keys exist in the file and the values are not empty.

	if (!ini_file)
		return false;

	IniParser ini_parser;

	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return false;

	char keymaps_value[128];

	for (int i=0; i<gs_settingsEntriesSize; i++){

		if (gs_list[i].isHeader)
			continue;
		
		// Check if key exist.
		if (ini_parser.getKeyValue(INI_FILE_SEC_SETTINGS, gs_list[i].key_ini_name.c_str(), keymaps_value) == INI_PARSER_KEY_NOT_FOUND)
			return false;

		// Check if value exist.
		if (strlen(keymaps_value) == 0)
			return false;
	}

	return true;
}

bool Settings::settingsPopulatedInFile(const char* ini_file)
{
	// Returns true if all settings that are found in the file have values (not empty).

	if (!ini_file)
		return false;

	IniParser ini_parser;

	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return false;

	if (!ini_parser.valuesOccupied(INI_FILE_SEC_SETTINGS))
		return false;

	return true;
}


void Settings::loadDefSettingsArray()
{
	
	if (!m_defSettings){
		m_defSettings = new string[gs_settingsEntriesSize];
	}

	IniParser ini_parser;
	
	if (ini_parser.init(DEF_CONF_FILE_PATH) != INI_PARSER_OK)
		return;

	char key_value[128];

	for (int i=0; i<gs_settingsEntriesSize; i++){

		if (gs_list[i].isHeader)
			continue;

		if (ini_parser.getKeyValue(INI_FILE_SEC_SETTINGS, gs_list[i].key_ini_name.c_str(), key_value) == INI_PARSER_KEY_NOT_FOUND) 
			continue;
		
		m_defSettings[i] = key_value;

		if (gs_list[i].id == CPU_SPEED) // % is not in ini file (HACK).
			m_defSettings[i].append("%");
	}
}

int	Settings::cmpSettingsToDef()
{
	// Returns 0 if current settings equal to default settings.

	if (!m_defSettings){
		return -1;
	}

	for (int i=0; i<gs_settingsEntriesSize; i++){

		if (gs_list[i].isHeader)
			continue;

		if (gs_list[i].value != m_defSettings[i])
			return 1;
	}

	return 0;
}

void Settings::settingsLoaded()
{
	// Notification that client has loaded new settings.
	m_userChanges = false;
}



