
/* settings.h: Program settings manager.

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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "view.h"
#include "navigator.h"
#include "scroll_bar.h"
#include "iRenderable.h"
#include <vector>
#include <string>

using std::string;
using std::vector;

typedef enum {
	STN_STATE_DEFAULT_CONF = 0, 
	STN_STATE_INGAME_DEFAULT_CONF, 
	STN_STATE_GAME_CONF, 
	STN_STATE_SELECTING, 
	STN_STATE_DEFAULT_MOD, 
	STN_STATE_INGAME_MOD
} SettingsState;

typedef enum {
	STN_ACTION_SAVE = 0, 
	STN_ACTION_SAVE_AS_DEFAULT, 
	STN_ACTION_LOAD_DEFAULT, 
	STN_ACTION_SELECT, 
	STN_ACTION_BACK, 
	STN_ACTION_EXIT
} SettingsAction;



class Controller;
class vita2d_texture;
class Settings : public Navigator, IRenderable
{

private:
	View*					m_view;
	Controller*				m_controller;
	string*					m_defSettings;
	SettingsState			m_state;
	int						m_highlight;
	int						m_borderTop;
	int						m_borderBottom;
	int						m_textColor;
	int						m_highlightColor;
	ScrollBar				m_scrollBar;
	string					m_aspectRatio;
	string					m_textureFilter;
	string					m_keyboardMode;
	int						m_highligtBarYpos;
	int						m_posXValue;
	int						m_maxValueWidth;
	string					m_saveDir;
	string					m_gameFileHeader;
	string					m_confFileDesc;
	RetCode					m_exitCode;
	bool					m_userChanges;
	
	void					show();
	void					render();
	void					renderInstructions();
	string					showValuesListBox(const char** values, int size);
	string					getDisplayFitString(const char* str, int limit, float font_size = 1.0);
	bool					isActionAllowed(SettingsAction);
	void					handleModelSetting(int key, const char* value);
	void					handleViewSetting(int key, const char* value);
	bool					fileExist(const char* file_name);
	string					getDirOfFile(const char* file);
	void					changeState();
	void					loadDefSettingsArray();
	int						cmpSettingsToDef();

	// Virtual function implementations
	bool					isExit(int buttons); 
	void					navigateUp(); 
	void					navigateDown(); 
	void					navigateRight(); 
	void					buttonReleased(int button);

public:
							Settings();
							~Settings();

	void					init(View* view, Controller* controller);
	RetCode					doModal(const char* save_dir, const char* game_file);
	string					getKeyValue(int key);
	void					getKeyValues(int key, const char** value, const char** src, const char*** values, int* size);
	void					setKeyValue(int key, const char* value, const char* src, const char** values, int size, int mask);
	void					setKeyValue(const char* key, const char* value, const char* src, const char** values, int size, int mask);
	void					applySetting(int key);
	void					applySettings(int group);
	void					createConfFile(const char* file);
	void					loadSettingsFromFile(const char* ini_file);
	void					saveSettingsToFile(const char* ini_file, bool over_write = true);
	bool					settingsPopulatedInFile(const char* ini_file);
	bool					settingsExistInFile(const char* ini_file);
	void					settingsLoaded();
	string					toString(int type);
};


#endif