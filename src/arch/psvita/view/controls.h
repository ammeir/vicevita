
/* controls.h: Control mappings manager.

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

#ifndef CONTROLS_H
#define CONTROLS_H

#include "control_pad.h"
#include "navigator.h"
#include "scroll_bar.h"
#include "iRenderable.h"
#include <vita2d.h>
#include <string>
#include <vector>

using std::string;
using std::vector;


typedef enum {
	CTRL_STATE_DEFAULT_CONF, 
	CTRL_STATE_INGAME_DEFAULT_CONF, 
	CTRL_STATE_GAME_CONF, 
	CTRL_STATE_SELECTING, 
	CTRL_STATE_DEFAULT_MOD, 
	CTRL_STATE_INGAME_MOD
} ControlsState;

typedef enum {
	CTRL_ACTION_SAVE, 
	CTRL_ACTION_SAVE_AS_DEFAULT, 
	CTRL_ACTION_LOAD_DEFAULT, 
	CTRL_ACTION_SELECT, 
	CTRL_ACTION_BACK, 
	CTRL_ACTION_EXIT
} ControlsAction;

typedef enum {
	ANALOG_UP, ANALOG_DOWN, ANALOG_LEFT, ANALOG_RIGHT, 
	DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, 
	CROSS, SQUARE, TRIANGLE, CIRCLE,
	SELECT, START, LTRIGGER, RTRIGGER, 
	LTRIGGER_RTRIGGER, LTRIGGER_CROSS, LTRIGGER_SQUARE, LTRIGGER_TRIANGLE, 
	LTRIGGER_CIRCLE, LTRIGGER_SELECT, LTRIGGER_START
} KeyIndex;


typedef struct{
	int					size;
	vita2d_texture*		arr[3];
	vita2d_texture*		highlight_arr[3];
	int					x_offset[3];
	int					y_offset[3];
} BitmapInfo;

#define JOYSTICK_UP		131
#define JOYSTICK_DOWN	132
#define JOYSTICK_LEFT	133
#define JOYSTICK_RIGHT  134
#define JOYSTICK_FIRE	135


extern vector<BitmapInfo> g_controlBitmaps;

class Controller;
class Settings;
class Controls : public Navigator, IRenderable
{

private:
	Controller*		m_controller;
	Settings*		m_settings;
	vector<string>	m_list;
	ControlPadMap*  m_mapLookup;
	ControlsState	m_state;
	int				m_highlight;
	int				m_borderTop;
	int				m_borderBottom;
	int				m_textColor;
	int				m_highlightColor;
	ScrollBar		m_scrollBar;
	string			m_saveDir;
	string			m_gameFile;
	string			m_confFileDesc;
	

	void			show();
	void			render();
	void			renderBitmaps();
	void			renderList();
	void			renderInstructions();
	string			showValuesListBox(const char** values, int size);
	bool			fileExist(string& file_name);
	bool			keyMapsValueExist(const char* ini_file);
	void			createConfFile(const char* ini_file);
	void			createGameSaveDir(const char* save_dir);
	string			getConfFileDesc();
	void			setMappingValue(int item_index, string& val);
	void			getMappingValue(int item_index, string& ret);
	bool			isActionAllowed(ControlsAction);
	void			updateKeyMapTable(vector<string>& updateVec);
	void			saveKeyMapTable(const char* file_path);
	void			setState(ControlsState state);
	void			loadResources();
	string			getDisplayFitString(const char* str, int limit, float font_size = 1);
	
	// Virtual function implementations (Navigator)
	bool			isExit(int buttons); 
	void			navigateUp(); 
	void			navigateDown(); 
	void			navigateRight(); 
	void			buttonReleased(int button);
	
public:
					Controls();
					~Controls();

	void			init(Controller*b, Settings*);
	void			doModal(const char* save_dir, const char* game_file);
	void			fillMappingValuesFile(const char* ini_file);
	void			fillMappingValuesBuf(const char* buffer);
	int				nameToMid(char* name);
	const char*		midToName(int mid);
	ControlPadMap*  getMappedKeyDigital(int button, int realBtnMask);
	ControlPadMap*  getMappedKeyAnalog(int analogDir);
	string			toString();
};


/*
             MAP LOOKUP TABLE
             ----------------------------------------
Values       |'a'| 49| 1 | 13| 2 |
             -----------------------------------------
KeyIndex     | 0 | 1 | 2 | 3 | 4 | 5 | 6 | ... | 20 |
             ----------------------------------------

			KeyIndex values:
			enum values above (KeyIndex)

			Matrix values:
            0-125 = Values in C64 keyboard matrix
			        MSN = row
					LSN = column

			56    = Restore (special value)
			24    = Shift lock (special value)

			Additional values:
			126   = None
            127   = Main Menu
            128   = Show/Hide Keyboard
			129   = Pause
			130   = Swap Joysticks
			131   = Joystick Up
			132   = Joystick Down
			133   = Joystick Left
			134   = Joystick Right
			135   = Joystick Fire
			  
			  
			# C64 keyboard matrix:
			#
			#       +-----+-----+-----+-----+-----+-----+-----+-----+
			#       |Bit 0|Bit 1|Bit 2|Bit 3|Bit 4|Bit 5|Bit 6|Bit 7|
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 0| DEL |Retrn|C_L/R|  F7 |  F1 |  F3 |  F5 |C_U/D|
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 1| 3 # |  W  |  A  | 4 $ |  Z  |  S  |  E  | S_L |
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 2| 5 % |  R  |  D  | 6 & |  C  |  F  |  T  |  X  |
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 3| 7 ' |  Y  |  G  | 8 ( |  B  |  H  |  U  |  V  |
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 4| 9 ) |  I  |  J  |  0  |  M  |  K  |  O  |  N  |
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 5|  +  |  P  |  L  |  -  | . > | : [ |  @  | , < |
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 6|POUND|  *  | ; ] | HOME| S_R |  =  | A_UP| / ? |
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+
			# |Bit 7| 1 ! |A_LFT| CTRL| 2 " |SPACE|  C= |  Q  | R/S |
			# +-----+-----+-----+-----+-----+-----+-----+-----+-----+



			Table values are populated from config.ini file at startup.
			keymaps value in the ini file correspond to the matrix id values described above.

*/


#endif