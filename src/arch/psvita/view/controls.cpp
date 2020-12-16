
/* controls.cpp: Control mappings manager.

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

#include "controls.h"
#include "view.h"
#include "controller.h"
#include "settings.h"
#include "guitools.h"
#include "texter.h"
#include "file_explorer.h"
#include "ini_parser.h"
#include "resources.h"
#include "app_defs.h"
#include "debug_psv.h"
#include <cstring>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h> 

// Max lines on the screen
#define MAX_ENTRIES 18
#define SCROLL_BAR_X 930
#define SCROLL_BAR_Y 38
#define SCROLL_BAR_WIDTH 8
#define SCROLL_BAR_HEIGHT 446


vector<BitmapInfo>	g_controlBitmaps;
static int gs_entriesSize = 23;
static int gs_mapValuesSize = 79;

// All control mapping values.
// If you add more values, remember to update gs_mapValuesSize, updateKeyMapTable() and PSV_ScanControls()
static const char* gs_valLookup[] = 
{
	"None","Main menu","Keyboard","Status bar","Pause","Reset","Swap joysticks","Warp mode","Joystick up","Joystick down","Joystick left",
	"Joystick right","Joystick fire","Joystick autofire","Cursor left/right", "Cursor up/down","Space","Return","F1","F3","F5",
	"F7","Clr/Home","Inst/Del","Ctrl","Restore","Run/Stop","C=","L Shift","R Shift","+","-","Pound","@","*",
	"Arrow up","[","]","=","<",">","?","Arrow left","1","2","3","4","5","6","7","8","9","0","A","B","C","D",
	"E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"
};

// Corresponding id numbers
static int gs_idLookup[] = 
{
	125,126,127,138,128,137,129,130,    // None,Main Menu,Keyboard,Status bar,Pause,Reset,Swap joysticks,Warp mode,
	131,132,133,134,135,136,            // Joystick up,Joystick down,Joystick left,Joystick right,Joystick fire,Joystick autofire,
	2,7,116,1,4,5,6,3,					// C_L/R,C_U/D,SPACE,RETURN,F1,F3,F5,F7
	99,0,114,56,119,117,23,100,         // HOME,DEL,CTRL,RESTORE,R/S,C=,S_L,S_R
	80,83,96,86,97,102,85,98,           // +,-,POUND,@,*,A_UP,:,;
	101,87,84,103,113,112,115,16,       // =,<,>,?,A_LFT,1,2,3
	19,32,35,48,51,64,67,18,            // 4,5,6,7,8,9,0,A
	52,36,34,22,37,50,53,65,            // B,C,D,E,F,G,H,I
	66,69,82,68,71,70,81,118,           // J,K,L,M,N,O,P,Q
	33,21,38,54,55,17,39,49,            // R,S,T,U,V,W,X,Y,
	20,0,0,0,0,0                        // Z
};

static const char* gs_defMapValues[] = 
{
	"Joystick up",    "Joystick down",      "Joystick left", "Joystick right",  
	"Joystick up",    "Joystick down",      "Joystick left", "Joystick right", 
	"Joystick fire",  "Return",             "None",          "Space",  
	"Main menu",      "Keyboard",           "None",          "None",
    "None",           "None",               "None",          "None",
	"None",           "None",				"None"
};



Controls::Controls()
{
	m_controller = NULL;
	m_settings = NULL;
	m_mapLookup = NULL;
	m_defMidArray = NULL;
}

Controls::~Controls()
{
	for (int i=0; i<g_controlBitmaps.size(); ++i){
		for (int j=0; j<g_controlBitmaps[i].size; ++j){
			vita2d_free_texture(g_controlBitmaps[i].arr[j]);
			vita2d_free_texture(g_controlBitmaps[i].highlight_arr[j]);
		}
	}

	if (m_mapLookup)
		delete[] m_mapLookup;

	if (m_defMidArray){
		delete[] m_defMidArray;
	}
}


void Controls::init(Controller* controller, Settings* settings)
{
	m_controller = controller;
	m_settings = settings;
	m_highlight = 0;
	m_borderTop = 0;
	m_borderBottom = MAX_ENTRIES-1;
	m_mapLookup = new ControlPadMap[gs_entriesSize];
	m_userChanges = false;

	for(int i=0; i<gs_entriesSize; ++i){
		m_list.push_back(string(gs_defMapValues[i]));
		m_mapLookup[i].ind = i;
	}

	// Update the mapping lookup table.
	updateKeyMapTable(m_list);

	if (!mappingsExistInFile(DEF_CONF_FILE_PATH)){
		// First run, mappings are missing. Save map id values to the conf file.
		saveKeyMapTable(DEF_CONF_FILE_PATH);
	}
	else if (!mappingsUpdatedInFile(DEF_CONF_FILE_PATH)){
		// Mappings count don't match. This is a version update.
		loadMappingsFromFile(DEF_CONF_FILE_PATH);
		saveKeyMapTable(DEF_CONF_FILE_PATH);
	}
	else{
		loadMappingsFromFile(DEF_CONF_FILE_PATH);
	}

	loadDefMidArray();
	loadResources();

	m_scrollBar.init(SCROLL_BAR_X, SCROLL_BAR_Y, SCROLL_BAR_WIDTH, SCROLL_BAR_HEIGHT);
	m_scrollBar.setListSize(m_list.size(), MAX_ENTRIES);
	m_scrollBar.setBackColor(GREY);
	m_scrollBar.setBarColor(ROYAL_BLUE);

	vita2d_set_clear_color(BLACK);
}

void Controls::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();
	
	vita2d_end_drawing();
	vita2d_wait_rendering_done();
	vita2d_swap_buffers();
}

void Controls::render()
{
	// File name
	txtr_draw_text(15, 20, C64_BLUE, m_gameFileHeader.c_str());
	// Conf file type
	txtr_draw_text(855, 20, C64_BLUE, m_confFileDesc.c_str());
	// Top seperation line
	vita2d_draw_line(15, 30, 940, 30, YELLOW_TRANSPARENT);

	renderList();
	renderBitmaps();
	// Bottom seperation line
	vita2d_draw_line(15, 495, 940, 495, YELLOW_TRANSPARENT);
	renderInstructions();
}

void Controls::doModal(const char* save_dir, const char* file_name)
{
	m_saveDir = save_dir;
	changeState();
	// Game file header name. Shrink the string to fit the screen.
	int max_width = 890 - txtr_get_text_width(m_confFileDesc.c_str(), 22);
	m_gameFileHeader = getDisplayFitString(file_name, max_width);

	show();
	scanCyclic();
}

void Controls::buttonReleased(int button)
{
	if(button == SCE_CTRL_SQUARE){ 
		// Save key mappings to game specific file

		if (!isActionAllowed(CTRL_ACTION_SAVE))
			return;

		string conf_file_path;

		if (m_saveDir.empty())
			conf_file_path = DEF_CONF_FILE_PATH;
		else
			conf_file_path = m_saveDir + CONF_FILE_NAME;

		// Create config.ini if it doesn't exist
		if (!fileExist(conf_file_path)){
			createConfFile(conf_file_path.c_str());
		}

		gtShowMsgBoxNoBtn("Saving...", this);
		sceKernelDelayThread(750000);
		updateKeyMapTable(m_list);
		saveKeyMapTable(conf_file_path.c_str());

		if (conf_file_path == DEF_CONF_FILE_PATH){
			loadDefMidArray();
		}
		m_userChanges = false;
		changeState();
		show();
	}
	else if (button == SCE_CTRL_TRIANGLE){ 
		// Load default key mappings
		if (!isActionAllowed(CTRL_ACTION_LOAD_DEFAULT))
			return;
		loadMappingsFromFile(DEF_CONF_FILE_PATH);
		m_userChanges = true;
		changeState();
		show();
	}
}

bool Controls::isExit(int buttons)
{
	if (buttons == SCE_CTRL_CIRCLE || buttons == SCE_CTRL_LEFT){ // Previous menu
		return true;
	}

	return false;
}

void Controls::navigateUp()
{
	if (m_highlight > 0){
		if (m_highlight-- == m_borderTop){
			m_borderTop--;
			m_borderBottom--;
			m_scrollBar.scrollUp();
		}

		show();
	}
}

void Controls::navigateDown()
{
	if (m_highlight < (m_list.size()-1)){
		if (m_highlight++ == m_borderBottom){
			m_borderBottom++;
			m_borderTop++;
			m_scrollBar.scrollDown();
		}

		show();
	}
}

void Controls::navigateRight()
{
	ControlsState prev_state = m_state;
	m_state = CTRL_STATE_SELECTING; // User selecting state
	string selection = showValuesListBox(gs_valLookup, gs_mapValuesSize);
	m_state = prev_state;

	if (!selection.empty()){
		string current;
		getMappingValue(m_highlight, current);
		
		if (current != selection){
			setMappingValue(m_highlight, selection);
			updateKeyMapTable(m_list);
			m_userChanges = true;
			changeState();
		}
	}

	show();
}

void Controls::renderList()
{
	int y = 55;
	int text_color;
	
	int start = m_borderTop;
	int end = (m_list.size() > MAX_ENTRIES)? m_borderBottom: m_list.size()-1;

	for (int i=start; i<=end; ++i){
		text_color = YELLOW;
	
		if (i == m_highlight){
			text_color = WHITE;
			int textHeight = txtr_get_text_height(m_list[i].c_str(), 22);
			int textWidth = txtr_get_text_width(m_list[i].c_str(), 22);

			// Draw highlight rectangle
			vita2d_draw_rectangle(15, y-textHeight, 910, textHeight+4, ROYAL_BLUE);
			
			// Draw arrow right
			txtr_draw_text(150 + textWidth + 10, y, text_color, ">");
		}

		txtr_draw_text(150, y, text_color, m_list[i].c_str());
		y = y + 25;
	}

	// Scroll bar
	if (m_list.size() > MAX_ENTRIES)
		m_scrollBar.render();
}

void Controls::renderBitmaps()
{
	int y = 40;
	vita2d_texture* bitmap;

	int start = m_borderTop;
	int end = (m_list.size() > MAX_ENTRIES)? m_borderBottom+1: m_list.size();
	for (int i=start; i<end; ++i){
		if (g_controlBitmaps[i].size == 1){
			bitmap = (i == m_highlight)? g_controlBitmaps[i].highlight_arr[0]: g_controlBitmaps[i].arr[0];
			vita2d_draw_texture(bitmap, 20, y);
		}
		else{
			for (int j=0; j<g_controlBitmaps[i].size; ++j){
				bitmap = (i == m_highlight)? g_controlBitmaps[i].highlight_arr[j]: g_controlBitmaps[i].arr[j];
				vita2d_draw_texture(bitmap, 20+g_controlBitmaps[i].x_offset[j], y+g_controlBitmaps[i].y_offset[j]);
			}
		}
		
		y = y + 25;
	}
}

void Controls::renderInstructions()
{
	switch (m_state){

	case CTRL_STATE_DEFAULT_CONF:
		// Default controls shown. Game not loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 395, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 490, 510);
		txtr_draw_text(516, 523, LIGHT_GREY, "Exit");
		break;
	case CTRL_STATE_INGAME_DEFAULT_CONF:
		// Default controls shown. Game loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 325, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 420, 510);
		txtr_draw_text(446, 523, LIGHT_GREY, "Exit");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 506, 510);
		txtr_draw_text(531, 523, LIGHT_GREY, "Save");
		break;
	case CTRL_STATE_SELECTING:
		// User selecting new value
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_X], 395, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_DPAD_LEFT_BLUE], 490, 510);
		txtr_draw_text(516, 523, LIGHT_GREY, "Back");
		break;
	case CTRL_STATE_GAME_CONF:
		// Game opened. Customized controls shown.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 310, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_TRIANGLE_BLUE], 400, 510);
		txtr_draw_text(433, 523, LIGHT_GREY, "Load default");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 580, 510);
		txtr_draw_text(605, 523, LIGHT_GREY, "Exit");
		break;
	case CTRL_STATE_DEFAULT_MOD: // Controls modified. Game not loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 200, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_TRIANGLE_BLUE], 290, 510);
		txtr_draw_text(323, 523, LIGHT_GREY, "Load default");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 470, 510);
		txtr_draw_text(495, 523, LIGHT_GREY, "Exit");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 558, 510);
		txtr_draw_text(583, 523, LIGHT_GREY, "Save as default");
		break;
	case CTRL_STATE_INGAME_MOD: // Controls modified. Game loaded.
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_NAVIGATE_UP_DOWN_LEFT], 270, 510);
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_TRIANGLE_BLUE], 360, 510);
		txtr_draw_text(393, 523, LIGHT_GREY, "Load default");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 540, 510);
		txtr_draw_text(565, 523, LIGHT_GREY, "Exit");
		vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 625, 510);
		txtr_draw_text(650, 523, LIGHT_GREY, "Save");
		break;
	};
}

bool Controls::isActionAllowed(ControlsAction action)
{
	switch (action)
	{
	case CTRL_ACTION_SAVE:
		return (m_state == CTRL_STATE_INGAME_DEFAULT_CONF ||
				m_state == CTRL_STATE_DEFAULT_MOD ||
				m_state == CTRL_STATE_INGAME_MOD)? true: false;
	case CTRL_ACTION_LOAD_DEFAULT:
		return (m_state == CTRL_STATE_GAME_CONF || 
				m_state == CTRL_STATE_DEFAULT_MOD ||
				m_state == CTRL_STATE_INGAME_MOD)? true: false;
	default:
		return false;
	};
}

void Controls::loadMappingsFromFile(const char* ini_file)
{
	// Populate the map vector from a ini file source.
	// Ini file can be game specific or default.

	if (!ini_file)
		return;

	IniParser ini_parser;
	
	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return;

	char keymaps_value[128] = {0};
	int item_index = 0;

	ini_parser.getKeyValue(INI_FILE_SEC_CONTROLS, INI_FILE_KEY_KEYMAPS, keymaps_value);

	if (strlen(keymaps_value) == 0) // Empty value
		return;

	char* token = strtok(keymaps_value, ",");

	if (!token)
		return;
	
	while (token) {
		int iToken = atoi(token);
		const char* entry = midToName(iToken);
		
		if (!entry)
			entry = midToName(125); // Set to 'None' if value is unknown.
		
		if (item_index < m_list.size()){
			m_list[item_index++] = entry;
		}
		
		token = strtok(NULL, ",");
	}

	
	// Update the mapping lookup table to reflect the changes.
	if (item_index > 0){
		updateKeyMapTable(m_list);
		m_userChanges = false; // No need to show Save button in instructions.
	}
}

void Controls::loadMappingsFromBuf(const char* buffer)
{
	// Populate the map vector from buffer source.
	// Buffer is game specific as it originates from a snapshot file.

	if (!buffer)
		return;

	// Make a copy because strtok modifies the buffer. 
	int size = strlen(buffer);
	char* keymap_values = new char[size];
	strncpy(keymap_values, buffer, size);

	// Skip key if present.
	char* offset;
	if ((offset = strstr(keymap_values, "Keymaps^")))
		keymap_values = offset + 8;

	int item_index = 0;
	char* token = strtok((char*)keymap_values, ",");
	
	while (token) {
		int iToken = atoi(token);
		const char* entry = midToName(iToken);
		
		if (entry){
			if (item_index < m_list.size()){
				m_list[item_index] = entry;
			}
		}

		item_index++;
		token = strtok(NULL, ",");
	}

	if (item_index > 0){
		updateKeyMapTable(m_list);
		m_userChanges = false;
	}

	delete[] keymap_values;
}

bool Controls::mappingsUpdatedInFile(const char* ini_file)
{
	// Returns true if all current mappings exist int the file.

	bool ret = false;
	IniParser ini_parser;
	
	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return false;

	char keymaps_value[128] = {0};
	int token_count = 0;
	
	ini_parser.getKeyValue(INI_FILE_SEC_CONTROLS, INI_FILE_KEY_KEYMAPS, keymaps_value);
	
	char* token = strtok(keymaps_value, ",");

	while (token) {
		token_count++;
		token = strtok(NULL, ",");
	}

	if (token_count == gs_entriesSize)
		ret = true;

	return ret;
}

bool Controls::mappingsExistInFile(const char* ini_file)
{
	// Returns true if mappings key value exist int the file.

	IniParser ini_parser;
	
	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return false;

	char keymaps_value[128] = {0};
	ini_parser.getKeyValue(INI_FILE_SEC_CONTROLS, INI_FILE_KEY_KEYMAPS, keymaps_value);

	if (strlen(keymaps_value) == 0)
		return false;
	
	return true;
}

void Controls::getMappingValue(int item_index, string& ret)
{
	if (item_index >= m_list.size())
		return;

	ret = m_list[item_index];
}

void Controls::setMappingValue(int item_index, string& val)
{
	if (item_index >= m_list.size())
		return;

	m_list[item_index] = val;
}

void Controls::updateKeyMapTable(vector<string>& updateVec)
{

	for (int i=0; i<gs_entriesSize; ++i){
		for (int j=0; j<gs_mapValuesSize; ++j){
			if (updateVec[i] == gs_valLookup[j]){
				int mid = gs_idLookup[j];
				m_mapLookup[i].mid = mid;
				m_mapLookup[i].iskey = 0;
				m_mapLookup[i].isjoystick = 0;
				m_mapLookup[i].istouch = 0;
				m_mapLookup[i].ispress = 0;

				switch (mid){	
				case 0 ... 124:
					m_mapLookup[i].iskey = 1;
					break;
				case JOYSTICK_UP:
					m_mapLookup[i].isjoystick = 1;
					m_mapLookup[i].joypin = 0x01;
					break;
				case JOYSTICK_DOWN: 
					m_mapLookup[i].isjoystick = 1;
					m_mapLookup[i].joypin = 0x02; 
					break;
				case JOYSTICK_LEFT: 
					m_mapLookup[i].isjoystick = 1;
					m_mapLookup[i].joypin = 0x04; 
					break;
				case JOYSTICK_RIGHT: 
					m_mapLookup[i].isjoystick = 1;
					m_mapLookup[i].joypin = 0x08; 
					break;
				case JOYSTICK_FIRE:
					m_mapLookup[i].isjoystick = 1;
					m_mapLookup[i].joypin = 0x10;
					break;
				}

				j = gs_mapValuesSize;
			}
		}
	}
}

void Controls::saveKeyMapTable(const char* ini_file)
{
	if (!ini_file)
		return;

	IniParser ini_parser;
	string keymaps_value;

	if (ini_parser.init(ini_file) != INI_PARSER_OK)
		return;

	char buf[32];

	// Keymap table to string.
	for (int i=0;i<gs_entriesSize; ++i){
		sprintf(buf, "%d",  m_mapLookup[i].mid); // sprintf adds a terminating null character after the content
		keymaps_value.append(buf);
		if (i < (gs_entriesSize - 1))
			keymaps_value.append(",");
	}

	ini_parser.setKeyValue(INI_FILE_SEC_CONTROLS, INI_FILE_KEY_KEYMAPS, keymaps_value.c_str());
	ini_parser.saveToFile(ini_file);
}

string Controls::showValuesListBox(const char** values, int size)
{
	return gtShowListBox(380, 125, 190, 300, values, size, this);
}

bool Controls::fileExist(string& file_name) 
 {
    if (FILE *file = fopen(file_name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

void Controls::createConfFile(const char* ini_file)
{
	if (!m_settings || !ini_file)
		return;

	m_settings->createConfFile(ini_file);
}

void Controls::createGameSaveDir(const char* save_dir)
{
	FileExplorer fileExp;

	if (!fileExp.dirExist(save_dir))
		fileExp.makeDir(save_dir);
}

int Controls::nameToMid(char* name)
{
	for (int i=0; i<gs_mapValuesSize; ++i){
		if (!strcmp(gs_valLookup[i], name))
			return gs_idLookup[i];
	}
	
	return 0;
}

const char* Controls::midToName(int mid)
{
	for (int i=0; i<gs_mapValuesSize; ++i){
		if (gs_idLookup[i] == mid)
			return gs_valLookup[i];
	}

	return NULL;
}

ControlPadMap* Controls::getMappedKeyDigital(int button, int realBtnMask)
{
	ControlPadMap* ret = NULL;

	switch (button)
	{
		case SCE_CTRL_SELECT:
			if (!(SCE_CTRL_SELECT & realBtnMask)){ // Is release?
				// Check is this is a shifting release or individual release.
				if (SCE_CTRL_LTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_SELECT].ispress)
					ret = &m_mapLookup[LTRIGGER_SELECT];
				else
					ret = &m_mapLookup[SELECT];
				
				break;
			}
			ret = (SCE_CTRL_LTRIGGER & realBtnMask)? &m_mapLookup[LTRIGGER_SELECT]: &m_mapLookup[SELECT];
			break;
		case SCE_CTRL_START:
			if (!(SCE_CTRL_START & realBtnMask)){
				if (SCE_CTRL_LTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_START].ispress)
					ret = &m_mapLookup[LTRIGGER_START];
				else
					ret = &m_mapLookup[START];
				
				break;
			}
			ret = (SCE_CTRL_LTRIGGER & realBtnMask)? &m_mapLookup[LTRIGGER_START]: &m_mapLookup[START];
			break;
		case SCE_CTRL_UP:
			ret = &m_mapLookup[DPAD_UP];
			break;
		case SCE_CTRL_RIGHT:
			ret = &m_mapLookup[DPAD_RIGHT];
			break;
		case SCE_CTRL_DOWN:
			ret = &m_mapLookup[DPAD_DOWN];
			break;
		case SCE_CTRL_LEFT:
			ret = &m_mapLookup[DPAD_LEFT];
			break;
		case SCE_CTRL_LTRIGGER:
			if (!(SCE_CTRL_LTRIGGER & realBtnMask)){ // Is ltrigger release?
				// Check if this is a shifting release.
				if (SCE_CTRL_RTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_RTRIGGER].ispress) 
					ret = &m_mapLookup[LTRIGGER_RTRIGGER];
				else if (SCE_CTRL_TRIANGLE & realBtnMask && m_mapLookup[LTRIGGER_TRIANGLE].ispress) 
					ret = &m_mapLookup[LTRIGGER_TRIANGLE];
				else if (SCE_CTRL_CIRCLE & realBtnMask && m_mapLookup[LTRIGGER_CIRCLE].ispress) 
					ret = &m_mapLookup[LTRIGGER_CIRCLE];
				else if (SCE_CTRL_CROSS & realBtnMask && m_mapLookup[LTRIGGER_CROSS].ispress) 
					ret = &m_mapLookup[LTRIGGER_CROSS];
				else if (SCE_CTRL_SQUARE & realBtnMask && m_mapLookup[LTRIGGER_SQUARE].ispress) 
					ret = &m_mapLookup[LTRIGGER_SQUARE];
				else if (SCE_CTRL_SELECT & realBtnMask && m_mapLookup[LTRIGGER_SELECT].ispress) 
					ret = &m_mapLookup[LTRIGGER_SELECT];
				else if (SCE_CTRL_START & realBtnMask && m_mapLookup[LTRIGGER_START].ispress) 
					ret = &m_mapLookup[LTRIGGER_START];
				else
					ret = &m_mapLookup[LTRIGGER];

				break;
			}

			ret = &m_mapLookup[LTRIGGER];
			
			break;
		case SCE_CTRL_RTRIGGER:
			if (!(SCE_CTRL_RTRIGGER & realBtnMask)){
				if (SCE_CTRL_LTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_RTRIGGER].ispress)
					ret = &m_mapLookup[LTRIGGER_RTRIGGER];
				else
					ret = &m_mapLookup[RTRIGGER];
				
				break;
			}
			ret = (SCE_CTRL_LTRIGGER & realBtnMask)? &m_mapLookup[LTRIGGER_RTRIGGER]: &m_mapLookup[RTRIGGER];
			break;
		case SCE_CTRL_L1:
			break;
		case SCE_CTRL_R1:
			break;
		case SCE_CTRL_TRIANGLE:
			if (!(SCE_CTRL_TRIANGLE & realBtnMask)){ 
				if (SCE_CTRL_LTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_TRIANGLE].ispress)
					ret = &m_mapLookup[LTRIGGER_TRIANGLE];
				else
					ret = &m_mapLookup[TRIANGLE];
				
				break;
			}
			ret = (SCE_CTRL_LTRIGGER & realBtnMask)? &m_mapLookup[LTRIGGER_TRIANGLE]: &m_mapLookup[TRIANGLE];
			break;
		case SCE_CTRL_CIRCLE:
			if (!(SCE_CTRL_CIRCLE & realBtnMask)){
				if (SCE_CTRL_LTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_CIRCLE].ispress)
					ret = &m_mapLookup[LTRIGGER_CIRCLE];
				else
					ret = &m_mapLookup[CIRCLE];
				
				break;
			}
			ret = (SCE_CTRL_LTRIGGER & realBtnMask)? &m_mapLookup[LTRIGGER_CIRCLE]: &m_mapLookup[CIRCLE];
			break;
		case SCE_CTRL_CROSS:
			if (!(SCE_CTRL_CROSS & realBtnMask)){
				if (SCE_CTRL_LTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_CROSS].ispress)
					ret = &m_mapLookup[LTRIGGER_CROSS];
				else
					ret = &m_mapLookup[CROSS];
				
				break;
			}
			ret = (SCE_CTRL_LTRIGGER & realBtnMask)? &m_mapLookup[LTRIGGER_CROSS]: &m_mapLookup[CROSS];
			break;
		case SCE_CTRL_SQUARE:
			if (!(SCE_CTRL_SQUARE & realBtnMask)){
				if (SCE_CTRL_LTRIGGER & realBtnMask && m_mapLookup[LTRIGGER_SQUARE].ispress)
					ret = &m_mapLookup[LTRIGGER_SQUARE];
				else
					ret = &m_mapLookup[SQUARE];
				
				break;
			}
			ret = (SCE_CTRL_LTRIGGER & realBtnMask)? &m_mapLookup[LTRIGGER_SQUARE]: &m_mapLookup[SQUARE];
			break;
		case SCE_CTRL_VOLUP:
			break;
		case SCE_CTRL_VOLDOWN:
			break;
	} 

	return ret;
}

ControlPadMap* Controls::getMappedKeyAnalog(int analogDir)
{
	return &m_mapLookup[analogDir];
}

string Controls::toString()
{
	string ret;
	char buf[16];

	ret.append(INI_FILE_KEY_KEYMAPS);
	ret.append(SNAP_MOD_DELIM_FIELD);
					
	// Keymap table to string.
	for (int i=0;i<gs_entriesSize; ++i){
		sprintf(buf, "%d",  m_mapLookup[i].mid); // sprintf adds a terminating null character
		ret.append(buf);
		if (i < (gs_entriesSize - 1))
			ret.append(",");
	}

	return ret;
}

void Controls::loadResources()
{
	BitmapInfo bi;
	bi.size = 1;
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_up_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_up_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_down_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_down_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_left_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_left_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_right_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_analog_right_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_up_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_up_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_down_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_down_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_left_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_left_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_right_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_dpad_right_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_cross_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_cross_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_square_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_square_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_triangle_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_triangle_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_circle_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_circle_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_select_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_select_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_start_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_start_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	g_controlBitmaps.push_back(bi);
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_rtrigger_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_rtrigger_blue);
	g_controlBitmaps.push_back(bi);
	
	// Combination bitmaps
	// L + R
	bi.size = 3; bi.x_offset[0] = 0; bi.x_offset[1] = 35; bi.x_offset[2] = 47;
	bi.y_offset[0] = 0; bi.y_offset[1] = 5; bi.y_offset[2] = 0;
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_black);
	bi.arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_rtrigger_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	bi.highlight_arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_blue);
	bi.highlight_arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_rtrigger_blue);
	g_controlBitmaps.push_back(bi);
	// L + CROSS
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_black);
	bi.arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_cross_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	bi.highlight_arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_blue);
	bi.highlight_arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_cross_blue);
	g_controlBitmaps.push_back(bi);
	// L + SQUARE
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_black);
	bi.arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_square_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	bi.highlight_arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_blue);
	bi.highlight_arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_square_blue);
	g_controlBitmaps.push_back(bi);
	// L + TRIANGLE
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_black);
	bi.arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_triangle_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	bi.highlight_arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_blue);
	bi.highlight_arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_triangle_blue);
	g_controlBitmaps.push_back(bi);
	// L + CIRCLE
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_black);
	bi.arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_circle_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	bi.highlight_arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_blue);
	bi.highlight_arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_circle_blue);
	g_controlBitmaps.push_back(bi);
	// L + SELECT
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_black);
	bi.arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_select_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	bi.highlight_arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_blue);
	bi.highlight_arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_select_blue);
	g_controlBitmaps.push_back(bi);
	// L + START
	bi.arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_black);
	bi.arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_black);
	bi.arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_start_black);
	bi.highlight_arr[0] = vita2d_load_PNG_buffer(img_ctrl_btn_ltrigger_blue);
	bi.highlight_arr[1] = vita2d_load_PNG_buffer(img_ctrl_plus_blue);
	bi.highlight_arr[2] = vita2d_load_PNG_buffer(img_ctrl_btn_start_blue);
	g_controlBitmaps.push_back(bi);
}

string Controls::getDisplayFitString(const char* str, int limit, float font_size)
{
	// Returns a shrinked string that fits to the limit boundaries.

	string ret;

	if (!str || !limit)
		return ret;

	ret = str;
	int str_width = txtr_get_text_width(str, font_size);

	if (str_width <= limit)
		return ret;

	while(str_width > limit){
		ret.pop_back();
		str_width = txtr_get_text_width(ret.c_str(), font_size);
	}

	ret.append("...");

	return ret;
}

void Controls::changeState()
{
	int diff = cmpMidsToDef();

	if (m_saveDir.empty()){ 
		// This is a case when the app was just started and no game/basic is yet loaded.
		m_state = (diff)? CTRL_STATE_DEFAULT_MOD: CTRL_STATE_DEFAULT_CONF;
		m_confFileDesc = "[Default]";
		return;
	}

	// Game or Basic loaded.
	if (diff){
		m_state = m_userChanges? CTRL_STATE_INGAME_MOD: CTRL_STATE_GAME_CONF;
		m_confFileDesc = "[Custom]";
	}else{
		m_state = m_userChanges? CTRL_STATE_INGAME_DEFAULT_CONF: CTRL_STATE_DEFAULT_CONF;
		m_confFileDesc = "[Default]";
	}
}

void Controls::loadDefMidArray()
{
	// Parse default key mappings into a map id array.

	if (!m_defMidArray){
		m_defMidArray = new int[gs_entriesSize];
	}

	IniParser ini_parser;
	
	if (ini_parser.init(DEF_CONF_FILE_PATH) != INI_PARSER_OK)
		return;

	char keymaps_value[128] = {0};
	int item_index = 0;

	ini_parser.getKeyValue(INI_FILE_SEC_CONTROLS, INI_FILE_KEY_KEYMAPS, keymaps_value);

	if (strlen(keymaps_value) == 0) // Empty value
		return;

	char* token = strtok(keymaps_value, ",");

	if (!token)
		return;
	
	int i = 0;
	while (token) {
		m_defMidArray[i++] = atoi(token);
		token = strtok(NULL, ",");
	}
}

int	Controls::cmpMidsToDef()
{
	// Returns 0 if current mappings equal to default mappings.

	if (!m_defMidArray)
		return -1;

	for (int i=0;i<gs_entriesSize; ++i){
		if (m_mapLookup[i].mid != m_defMidArray[i])
			return 1;
	}

	return 0;
}

