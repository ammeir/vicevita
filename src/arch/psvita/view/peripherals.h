
/* peripherals.h: Peripheral settings and control class.

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

#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "view.h"
#include "navigator.h"
#include "scroll_bar.h"
#include "iRenderable.h"
#include <vector>
#include <string>


typedef enum {
	PERIF_ACTION_SAVE = 0,
	PERIF_ACTION_LOAD,
	PERIF_ACTION_ATTACH, 
	PERIF_ACTION_DETACH,
	PERIF_ACTION_FREEZE
} PeripheralsAction;


class Controller;
class vita2d_texture;
class Peripherals : public Navigator, IRenderable
{

private:
	View*					m_view;
	Controller*				m_controller;
	int						m_highlight;
	int						m_borderTop;
	int						m_borderBottom;
	int						m_textColor;
	int						m_highlightColor;
	ScrollBar				m_scrollBar;
	bool					m_settingsChanged;
	bool					m_selectingValue;
	int						m_highligtBarYpos;
	int						m_posXValue;
	int						m_maxValueWidth;
	string					m_devSaveFiles[6];
	RetCode					m_exitCode;
	
	// Virtual function implementations
	bool					isExit(int buttons); 
	void					navigateUp(); 
	void					navigateDown(); 
	void					navigateRight(); 
	void					buttonReleased(int button);

	void					show();
	void					render();
	void					renderInstructions();
	void					loadSettingsFromFile(const char* ini_file);
	void					saveSettingsToFile(const char* ini_file);
	string					showValuesListBox(const char** values, int size);
	string					showFileBrowser(int peripheral);
	string					getDisplayFitString(const char* str, int limit, float font_size = 1);
	bool					isActionAllowed(PeripheralsAction);
	bool					naviOnPeripheral();
	bool					naviOnSetting();
	void					handleModelSetting(int key, const char* value);
	void					handleViewSetting(int key, const char* value);
	string					getImageFileName();
	void					detachImage(int peripheral);
	int						getKeyIndex(int key);
	int						getValueIndex(const char* value, const char** values, int size);
	int						getDriveId();


public:
							Peripherals();
							~Peripherals();

	void					init(View* view, Controller* controller);
	RetCode					doModal();
	int						loadImage(int load_type, const char* file, int index = 0);
	string					getKeyValue(int key);
	void					getKeyValues(int key, const char** value, const char** value2, const char*** values, int* size);
	void					setKeyValue(int key, const char* value, const char* value2, const char** values, int size, int mask);
	int						attachImage(int peripheral, const char* image);
	void					applyAllSettings();
	void					notifyReset();
};


#endif