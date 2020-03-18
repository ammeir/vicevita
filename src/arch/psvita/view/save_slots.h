
/* save_slots.h: Savestate implementation.
   
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

#ifndef SAVE_SLOTS_H
#define SAVE_SLOTS_H

#include "view.h"
#include "file_explorer.h"
#include "navigator.h"
#include "iRenderable.h"
#include <vector>
#include <string>

using std::string;
using std::vector;


enum SaveSlotsState {
	SAVESLOTS_INITIAL = 0, 
	SAVESLOTS_INGAME_NOSAVES, 
	SAVESLOTS_INGAME_SAVES
};

struct GridEntry
{
	int posX;
	int posY;
	int height;
	int width;
	string text;
	string file_path;
	string time_stamp;
	vita2d_texture* thumb_img;
};

class View;
class Controller;
class Controls;
class Settings;
class SceDateTime;
class SaveSlots : public Navigator, IRenderable
{

private:
	View*				m_view;
	Controller*			m_controller;
	Controls*			m_controls;
	Settings*			m_settings;
	int					m_gridPosX;
	int					m_gridPosY;
	int					m_gridWidth;
	int					m_gridHeight;
	GridEntry			m_grid[2][3];
	int					m_highlightSlot;
	int					m_textColor;
	int					m_highlightColor;
	string				m_path;
	SaveSlotsState		m_state;
	string				m_loadFilePath;
	string				m_displayfileName;
	RetCode				m_exitCode;

	
	void				show();
	virtual	void		render();
	void				resetGrid();
	void				initiateGrid();
	void				populateGrid();
	void				drawThumbnails();
	void				drawSlotTexts();
	void				drawTimeStamps();
	void				drawHighlightSquare();
	void				drawInstructions();
	void				waitTillButtonsReleased();
	void				setState();
	int					saveViewToThumbnail(const char* fname);
	int					saveAsPng(unsigned char* image, const char* filename, int width, int height);
	int					touchCoordinatesToSaveSlot(int x, int y);
	bool				isSlotOccupied(int slot);
	bool				isGridEmpty();
	bool				confirmUser(const char* msg);
	void				emptySaveSlot(int slot);
	void				addTimeStamp(int slot, char* time_stamp);
	int					addThumbToSnap(const char* snap_file);
	int					addSettingsToSnap(const char* snap_file);
	void				changeHighlightSquare(int button);
	string				getfilePath(int slot);
	string				formatTimeStamp(string seconds);
	string				getSnapshotFromDirContent(vector<DirEntry> &dir, int save_slot);
	const char*			getThumbFromSnap(const char* file);
	int					getSettingsFromSnap(const char* file, char** data);
	string				getTimeStampFromDirContent(vector<DirEntry> &dir, int save_slot);
	string				getDisplayFitString(const char* str, int limit, float font_size = 1);
	void				cleanUp();
	char*				readFileToBuf(const char* file, long* file_size);
	int					applyPatchModuleSettings(const char* snapshot);

	// Navigator interface implementations
	bool				isExit(int buttons); 
	void				navigateUp(); 
	void				navigateDown(); 
	void				navigateLeft(); 
	void				navigateRight(); 
	void				buttonReleased(int button);

public:
						SaveSlots();
	virtual				~SaveSlots();
	void				init(View*, Controller*, Controls*, Settings*, int posX, int posY, int width, int height);
	RetCode				doModal(const char* save_path, const char* file_name);
};


#endif