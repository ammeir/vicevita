
/* save_slots.cpp: Savestate implementation.

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

#include "save_slots.h"
#include "controller.h"
#include "controls.h"
#include "settings.h"
#include "texter.h"
#include "guitools.h"
#include "app_defs.h"
#include "resources.h"
#include "debug_psv.h"

#include <ctime>
#include <sstream> /* std::istringstream */
#include <vita2d.h>
#include <png.h>
#include <psp2/rtc.h> 
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h> 


#define THUMBNAIL_WIDTH 320
#define THUMBNAIL_HEIGHT 200


SaveSlots::SaveSlots()
{
	for (int i=0; i<2; ++i){
		for (int j=0; j<3; ++j){
			GridEntry* slot = &m_grid[i][j];
			slot->posX = 0;
			slot->posY = 0;
			slot->width = 0;
			slot->height = 0;
			slot->thumb_img = 0;
		}
	}
}

SaveSlots::~SaveSlots()
{
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			GridEntry* slot = &m_grid[i][j];
			if (slot && slot->thumb_img)
				vita2d_free_texture(slot->thumb_img);
		}
	}
}

void SaveSlots::init(View* view, 
	                 Controller* controller, 
					 Controls* controls, 
					 Settings* settings,
					 int posX, int posY, int width, int height)
{
	m_view = view;
	m_controller = controller;
	m_controls = controls;
	m_settings = settings;
	m_gridPosX = posX;
	m_gridPosY = posY;
	m_gridWidth = width;
	m_gridHeight = height;
	m_highlightSlot = 1;

	initiateGrid();

	setState(); // Little state machine is used to keep track which instruction buttons to show.
	setNavJoyPins(0x00); // Disable joystick
}

RetCode SaveSlots::doModal(const char* save_path, const char* file_name)
{
	if (m_path != save_path)
		resetGrid();

	m_path = save_path;
	populateGrid();
	setState();
	m_displayfileName = getDisplayFitString(file_name, 930);
	m_exitCode = EXIT;
	show();
	scanCyclic();
	cleanUp();

	return m_exitCode;
}

void SaveSlots::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing(); // This actions all pending draws. Nothing is still visible.
	vita2d_swap_buffers(); // This will make all draws visible
	vita2d_wait_rendering_done();
}

void SaveSlots::render()
{
	int text_color = YELLOW;
	int line_color = YELLOW;
	
	// File name
	txtr_draw_text(m_gridPosX, m_gridPosY-10, C64_BLUE, m_displayfileName.c_str());
	
	// Draw the save slots
	vita2d_draw_line(m_gridPosX, m_gridPosY, m_gridPosX+m_gridWidth, m_gridPosY, line_color); // Top frame line
	vita2d_draw_line(m_gridPosX, m_gridPosY+m_gridHeight, m_gridPosX+m_gridWidth, m_gridPosY+m_gridHeight, line_color); // Bottom frame line
	vita2d_draw_line(m_gridPosX, m_gridPosY, m_gridPosX, m_gridPosY+m_gridHeight, line_color); // Left frame line
	vita2d_draw_line(m_gridPosX+m_gridWidth, m_gridPosY, m_gridPosX+m_gridWidth, m_gridPosY+m_gridHeight, line_color); // Right frame line
	vita2d_draw_line(m_gridPosX, m_gridPosY+(m_gridHeight/2), m_gridPosX+m_gridWidth, m_gridPosY+(m_gridHeight/2), line_color); // Horizontal divider
	vita2d_draw_line(m_gridPosX+(m_gridWidth/3), m_gridPosY, m_gridPosX+(m_gridWidth/3), m_gridPosY+m_gridHeight, line_color); // First vertical divider
	vita2d_draw_line(m_gridPosX+2*(m_gridWidth/3), m_gridPosY, m_gridPosX+2*(m_gridWidth/3), m_gridPosY+m_gridHeight, line_color); // Second vertical divider
	
	drawSlotTexts();
	drawThumbnails();
	drawTimeStamps();
	drawHighlightSquare();
	drawInstructions();
}

void SaveSlots::buttonReleased(int button)
{
	switch (button){
		case SCE_CTRL_TRIANGLE: // Empty the slot
			if (!isSlotOccupied(m_highlightSlot))
				break;
			if (confirmUser("Delete save state?")){
				emptySaveSlot(m_highlightSlot);
				setState();
			}
			show(); 
			break;
		case SCE_CTRL_CIRCLE: 
			break;
		case SCE_CTRL_CROSS: // Load
			if (isSlotOccupied(m_highlightSlot)){
				gtShowMsgBoxNoBtn("Loading...", this);
				sceKernelDelayThread(350000); // Just for the looks
				string snapshot = getfilePath(m_highlightSlot);
				
				if (m_controller->loadState(snapshot.c_str()) < 0){
					gtShowMsgBoxOk("Load failed", this);
					show();
					return;
				}
				// Show the Model changes in the settings.
				m_controller->syncModelSettings();
				// Apply the extra settings that are stored in the patch.
				applyPatchModuleSettings(snapshot.c_str());
				Navigator::m_running = false; 
				m_exitCode = EXIT_MENU; // Return to the game
			}
			break;
		case SCE_CTRL_SQUARE: // Save
		{
			// Do nothing if nothing is loaded
			if (g_game_file.empty())
				return;

			// Ask user confirmation if this is overwrite
			if (isSlotOccupied(m_highlightSlot)){
				if (!confirmUser("Overwrite existing save?")){
					show();
					return;
				}
				emptySaveSlot(m_highlightSlot);
			}

			char buf[64];
			time_t seconds;
			SceDateTime time_local;

			// Get time in GMT time zone. Later on we will convert it to local time.
			sceRtcGetCurrentClock(&time_local, 0); 
			sceRtcGetTime_t(&time_local, &seconds); // convert to seconds

			// Add time stamp to save file name
			sprintf(buf, "s%d%lu", m_highlightSlot, seconds); 
			string snap_file = m_path + buf;

			// Make sure directory exist
			FileExplorer fileExp;
			if (fileExp.readDirContent(m_path.c_str()) == RET_DIR_OPEN_ERROR)
				fileExp.makeDir(m_path.c_str());

			gtShowMsgBoxNoBtn("Saving...", this);
			
			// Save snaphot and patch it with thumbnail and settings.
			if (m_controller->saveState(snap_file.c_str()) < 0){
				gtShowMsgBoxOk("Save failed", this);
				show();
				break;
			}
			addThumbToSnap(snap_file.c_str());
			addSettingsToSnap(snap_file.c_str());
			populateGrid();
			setState();
			show();
			break;
		}
	}
}

bool SaveSlots::isExit(int buttons)
{
	//PSV_DEBUG("SaveSlots::isExit()");
	if (buttons == SCE_CTRL_CIRCLE){
		return true;
	}

	return false;
}

void SaveSlots::navigateUp()
{
	changeHighlightSquare(SCE_CTRL_UP);
	show();
}

void SaveSlots::navigateDown()
{
	changeHighlightSquare(SCE_CTRL_DOWN);
	show();
}

void SaveSlots::navigateLeft()
{
	// Let user exit with left button (from slots 1 and 4).
	if (m_highlightSlot == 1 || m_highlightSlot == 4)
		Navigator::m_running = false;
	
	changeHighlightSquare(SCE_CTRL_LEFT);
	show();
}

void SaveSlots::navigateRight()
{
	changeHighlightSquare(SCE_CTRL_RIGHT);
	show();
}

string SaveSlots::getSnapshotFromDirContent(vector<DirEntry> &dir, int save_slot)
{
	// Second character in the file name tells the number of the save slot. Slots start from 1 (not 0).
	// Subtract by 48 to get the integer number. E.g. char '1' is ascii 49, char '2' is 50...
	string ret;
	for (vector<DirEntry>::iterator it = dir.begin(); it != dir.end(); ++it){
		if ((*it).isFile && save_slot == ((*it).name[1] - 48) && (*it).name[0] == 's')
			ret = (*it).path;
	}	

	return ret;
}

int SaveSlots::addThumbToSnap(const char* snap_file)
{
	// Creates a png thumbnail image and patches it to the snapshot file.
	
	// TODO: Find a way how to create the png image straight to memory buffer.
	// Currently the png is written to a file and then read to buffer.

	if (!snap_file)
		return -1;
		
	int ret = 0;
	string thumb_file = m_path + "tmp_thumb";

	if (saveViewToThumbnail(thumb_file.c_str()) < 0){
		return -1;
	}

	long file_size = 0;
	char* file_buf = readFileToBuf(thumb_file.c_str(), &file_size);

	if (file_buf && file_size > 0){
		patch_data_s patch;
		patch.snapshot_file = snap_file;
		patch.module_name = SNAP_MOD_THUMB;
		patch.major = 1;
		patch.minor = 1;
		patch.data = file_buf;
		patch.data_size = file_size;
		if (m_controller->patchSaveState(&patch) < 0)
			ret = -1;
	}
	
	if (file_buf)
		delete[] file_buf;

	FileExplorer::getInst()->deleteFile(thumb_file.c_str());

	return ret;
}

const char* SaveSlots::getThumbFromSnap(const char* file)
{
	patch_data_s patch;
	patch.snapshot_file = file;
	patch.module_name = SNAP_MOD_THUMB;
	
	// Get the bitmap size. Not the fastest way but easy.
	if (m_controller->getSaveStatePatchInfo(&patch) < 0)
		return NULL;

	patch.data = new char[patch.data_size]; // Remember to deallocate.
	m_controller->getSaveStatePatch(&patch);
	return patch.data;
}

int	SaveSlots::getSettingsFromSnap(const char* file, char** data)
{
	patch_data_s patch;
	patch.snapshot_file = file;
	patch.module_name = SNAP_MOD_SETTINGS;
	
	if (m_controller->getSaveStatePatchInfo(&patch) < 0)
		return -1;

	patch.data = new char[patch.data_size + 1]; // One more for null character.
	
	if (m_controller->getSaveStatePatch(&patch) < 0){
		delete[] patch.data;
		return -1;
	}

	((char*)patch.data)[patch.data_size] = '\0';
	*data = (char*)patch.data;

	return 0;
}

int	SaveSlots::addSettingsToSnap(const char* snap_file)
{
	// Combine control keymaps and view settings into a patch module and patch the snap.
	// Format: keymaps^value|setting1^value|...|settingn^value

	if (!snap_file)
		return -1;
		
	int ret = 0;
	string settings = m_controls->toString();
	settings.append(SNAP_MOD_DELIM_ENTRY);
	settings.append(m_settings->toString(SETTINGS_VIEW).c_str());
	// Missing Model settings in the vanilla snap.
	settings.append(SNAP_MOD_DELIM_ENTRY);
	settings.append(m_settings->toString(SETTINGS_MODEL_NOT_IN_SNAP).c_str());

	patch_data_s patch;
	patch.snapshot_file = snap_file;
	patch.module_name = SNAP_MOD_SETTINGS;
	patch.major = 1;
	patch.minor = 1;
	patch.data = settings.c_str();
	patch.data_size = settings.size();
	
	if (m_controller->patchSaveState(&patch) < 0)
		ret = -1;
	
	return ret;
}

string SaveSlots::getTimeStampFromDirContent(vector<DirEntry> &dir, int save_slot)
{
	// Second character in the file name tells the number of the save slot. Slots start from 1 (not 0).
	// Subtract by 48 to get the integer number. E.g. char '1' is ascii 49, char '2' is 50...
	// Timestamp is after the slot number.
	string ret, seconds;
	for (vector<DirEntry>::iterator it = dir.begin(); it != dir.end(); ++it){
		if ((*it).isFile && save_slot == ((*it).name[1] - 48)){
			seconds = (*it).name.substr(2, string::npos);
			ret = formatTimeStamp(seconds);
		}
	}	

	return ret;
}

bool SaveSlots::isSlotOccupied(int slot)
{
	bool ret = false;

	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			int grid_number = (j+i*3);
			if (grid_number == slot-1){
				if (!m_grid[i][j].file_path.empty())
				ret = true;
			}
		}
	}

	return ret;
}

bool SaveSlots::isGridEmpty()
{
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			if (!m_grid[i][j].file_path.empty())
				return false;
		}
	}

	return true;
}

void SaveSlots::emptySaveSlot(int slot)
{
	FileExplorer fileExp;

	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			int grid_number = (j+i*3);
			GridEntry* grid_entry = &m_grid[i][j];
			if (grid_number == slot-1 && !grid_entry->file_path.empty()){
				fileExp.deleteFile(grid_entry->file_path.c_str());
				grid_entry->file_path.clear();
				grid_entry->time_stamp.clear();
				grid_entry->text = "Empty";
				
				if (grid_entry->thumb_img){
					vita2d_free_texture(grid_entry->thumb_img);
					grid_entry->thumb_img = NULL;
				}
			}
		}
	}
}

void SaveSlots::resetGrid()
{
	// Empty previous grid values
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			GridEntry* slot = &m_grid[i][j];
			slot->text.clear();
			slot->file_path.clear();
			slot->time_stamp.clear();
			slot->text = "Empty";
			if (slot->thumb_img){
				vita2d_free_texture(slot->thumb_img);
				slot->thumb_img = NULL;
			}
		}
	}
}

void SaveSlots::initiateGrid()
{
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			int slot_number = (j+i*3);
			GridEntry* slot = &m_grid[i][j];
			slot->posX = m_gridPosX + j*(m_gridWidth/3);
			slot->posY = m_gridPosY + i*(m_gridHeight/2);
			slot->width = m_gridWidth/3;
			slot->height = m_gridHeight/2;
			slot->text = "Empty";
		}
	}
}

void SaveSlots::populateGrid()
{
	// Fill grid with thumbnails and timestamps.

	FileExplorer fileExp;
	fileExp.readDirContent(m_path.c_str());
	fileExp.sortDirContent();
	vector<DirEntry> dir_content = fileExp.getDirContent();

	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			int slot_number = (j+i*3);
			GridEntry* slot = &m_grid[i][j];
			
			if (!slot->file_path.empty())
				continue;
		
			slot->file_path = getSnapshotFromDirContent(dir_content, slot_number+1);
			if (slot->file_path.empty())
				continue;

			slot->time_stamp = getTimeStampFromDirContent(dir_content, slot_number+1);
			const char* buf = getThumbFromSnap(slot->file_path.c_str());
			
			if (buf){
				slot->thumb_img = vita2d_load_PNG_buffer(buf);
				delete[] buf;
				continue;
			}
		
			slot->text = "No thumb";	
		}
	}
}

void SaveSlots::setState()
{
	m_state = g_game_file.empty()? SAVESLOTS_INITIAL: SAVESLOTS_INGAME_NOSAVES;

	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			if (!m_grid[i][j].file_path.empty()){
				m_state = SAVESLOTS_INGAME_SAVES;
				break;
			}
		}
	}
}

void SaveSlots::drawThumbnails()
{
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			GridEntry* grid = &m_grid[i][j];
			if (grid->file_path.empty()) 
				continue;
			if (!grid->thumb_img) 
				continue;
			vita2d_draw_texture_scale(grid->thumb_img, m_grid[i][j].posX, m_grid[i][j].posY, 
										(float)(m_grid[i][j].width-2) / THUMBNAIL_WIDTH, 1);
		}
	}
}

void SaveSlots::addTimeStamp(int slot, char* time_stamp)
{
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			int grid_number = (j+i*3);
			if (grid_number == slot-1 && !m_grid[i][j].file_path.empty()){
				m_grid[i][j].time_stamp = time_stamp;
			}
		}
	}
}

string SaveSlots::getfilePath(int slot)
{
	string ret;

	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			int grid_number = (j+i*3);
			if (grid_number == slot-1){
				ret = m_grid[i][j].file_path;
			}
		}
	}

	return ret;
}

void SaveSlots::drawSlotTexts()
{
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){
			txtr_draw_text(m_grid[i][j].posX+120, m_grid[i][j].posY+124, YELLOW_TRANSPARENT, m_grid[i][j].text.c_str());
		}
	}
}

void SaveSlots::drawTimeStamps()
{
	for (int i=0; i<2; i++){
		for (int j=0; j<3; j++){

			if (m_grid[i][j].time_stamp.empty())
				continue;
			txtr_draw_text(m_grid[i][j].posX+50, 
						   m_grid[i][j].posY+(m_gridHeight/2) - 7, 
						   YELLOW, 
						   m_grid[i][j].time_stamp.c_str());
		}
	}
}

string SaveSlots::formatTimeStamp(string seconds)
{
	string ret;
	time_t t;
	char buf[64];
	
	std::istringstream is(seconds);
	is >> t;

	struct tm* s_time = localtime(&t); // Conver GMT to local time

	sprintf(buf, "%04d-%02d-%02d  %02d:%02d:%02d", 
				 s_time->tm_year+1900, 
				 s_time->tm_mon+1, 
				 s_time->tm_mday, 
				 s_time->tm_hour, 
				 s_time->tm_min, 
				 s_time->tm_sec);

	ret = buf;
	return ret;
}


int SaveSlots::touchCoordinatesToSaveSlot(int x, int y)
{
	int ret = 0;
	if (y >= 0 && y <= 500) // First row
	{
		if (x<600)
			ret = 1;
		else if (x<1200)
			ret = 2;
		else
			ret = 3;
	}
	else // Second row
	{
		if (x<600)
			ret = 4;
		else if (x<1200)
			ret = 5;
		else
			ret = 6;
	}

	return ret;
}

void SaveSlots::changeHighlightSquare(int button)
{
	switch (m_highlightSlot)
	{
		case 1:
			if (button == SCE_CTRL_RIGHT){
				m_highlightSlot = 2;
			}
			else if (button == SCE_CTRL_DOWN){
				m_highlightSlot = 4;
			}
			break;
		case 2:
			if (button == SCE_CTRL_LEFT){
				m_highlightSlot = 1;
			}
			else if (button == SCE_CTRL_RIGHT){
				m_highlightSlot = 3;
			}
			else if (button == SCE_CTRL_DOWN){
				m_highlightSlot = 5;
			}
			break;
		case 3:
			if (button == SCE_CTRL_LEFT){
				m_highlightSlot = 2;
			}
			else if (button == SCE_CTRL_DOWN){
				m_highlightSlot = 6;
			}
			break;
		case 4:
			if (button == SCE_CTRL_RIGHT){
				m_highlightSlot = 5;
			}
			else if (button == SCE_CTRL_UP){
				m_highlightSlot = 1;
			}
			break;
		case 5:
			if (button == SCE_CTRL_LEFT){
				m_highlightSlot = 4;
			}
			else if (button == SCE_CTRL_RIGHT){
				m_highlightSlot = 6;
			}
			else if (button == SCE_CTRL_UP){
				m_highlightSlot = 2;
			}
			break;
		case 6:
			if (button == SCE_CTRL_LEFT){
				m_highlightSlot = 5;
			}
			else if (button == SCE_CTRL_UP){
				m_highlightSlot = 3;
			}
			break;
	}
}

void SaveSlots::drawHighlightSquare()
{
	GridEntry* slot;

	// Find the right grid slot
	switch (m_highlightSlot)
	{
		case 1:
			slot = &m_grid[0][0];
			break;
		case 2:
			slot = &m_grid[0][1];
			break;
		case 3:
			slot = &m_grid[0][2];
			break;
		case 4:
			slot = &m_grid[1][0];
			break;
		case 5:
			slot = &m_grid[1][1];
			break;
		case 6:
			slot = &m_grid[1][2];
			break;
		default:
			slot = &m_grid[0][0];
	}

	// Draw rectangle
	vita2d_draw_rectangle(slot->posX, slot->posY, slot->width-2, 6, C64_BLUE); // Top 
	vita2d_draw_rectangle(slot->posX, slot->posY+slot->height-6, slot->width-2, 5, C64_BLUE); // Bottom
	vita2d_draw_rectangle(slot->posX, slot->posY, 6, slot->height-2, C64_BLUE); // Left
	vita2d_draw_rectangle(slot->posX+slot->width-7, slot->posY, 6, slot->height-2, C64_BLUE); // Right  
}

void SaveSlots::drawInstructions()
{
	switch (m_state){

		case SAVESLOTS_INGAME_NOSAVES:
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 394, 515);
			txtr_draw_text(420, 528, LIGHT_GREY, "Save");
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 500, 515);
			txtr_draw_text(526, 528, LIGHT_GREY, "Exit");
			break;

		case SAVESLOTS_INGAME_SAVES:
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_SQUARE_MAGENTA], 272, 515);
			txtr_draw_text(294, 528, LIGHT_GREY, "Save");
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CROSS_BLUE], 372, 515);
			txtr_draw_text(393, 528, LIGHT_GREY, "Load");
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_TRIANGLE_BLUE], 470, 516);
			txtr_draw_text(503, 528, LIGHT_GREY, "Delete");
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 596, 515);
			txtr_draw_text(622, 528, LIGHT_GREY, "Exit");
			break;
		default:
			vita2d_draw_texture(g_instructionBitmaps[IMG_BTN_CIRCLE_BLUE], 433, 515);
			txtr_draw_text(459, 528, LIGHT_GREY, "Exit");
	};
}

bool SaveSlots::confirmUser(const char* msg)
{
	return gtShowMsgBoxOkCancel(msg, this);
}

void SaveSlots::waitTillButtonsReleased()
{
	SceCtrlData ctrl;

	sceCtrlPeekBufferPositive(0, &ctrl, 1); 

	if (ctrl.buttons){
		while(1){
			sceCtrlReadBufferPositive(0, &ctrl, 1);
			if (!ctrl.buttons){
				break;
			}
		}
	}
}

int SaveSlots::saveViewToThumbnail(const char* fname)
{
	if (!fname)
		return -1;

	unsigned char* thumb = m_view->getThumbnail();
	if (!thumb){
		return -1;
	}

	saveAsPng(thumb, fname, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);

	delete[] thumb;
	return 0;
}

int SaveSlots::saveAsPng(unsigned char *img, const char *filename, int width, int height)
{
	// Saves the thumbnail as a png file.

	FILE* fp = fopen(filename, "wb");
	if (fp == NULL) {
		return -1;
	}
	
	png_structp pngStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (pngStruct == NULL) {
		return -1;
	}
	
	png_infop pngInfo = png_create_info_struct(pngStruct);
	if (pngInfo == NULL) {
		png_destroy_write_struct(&pngStruct, NULL);
		return -1;
	}
	
	if (setjmp(png_jmpbuf(pngStruct))) {    
		png_destroy_write_struct(&pngStruct, &pngInfo);
		fclose(fp);
		return -1;
	}
	
	png_init_io(pngStruct, fp);
	png_set_IHDR(pngStruct, pngInfo, width, height, 8, 
					PNG_COLOR_TYPE_RGB,
					PNG_INTERLACE_NONE, 
					PNG_COMPRESSION_TYPE_DEFAULT, 
					PNG_FILTER_TYPE_DEFAULT);

	png_write_info(pngStruct, pngInfo);
	png_write_flush(pngStruct);
	
	png_uint_32 bytesInRow = width * 3;
	png_bytep* rows = (png_bytep*)new char[height * sizeof(png_bytep)];

	if (rows == NULL) {
		fclose(fp);
		png_destroy_write_struct(&pngStruct, &pngInfo);
		return -1;
	}

	for (int i = 0; i < height; i++) {
		rows[i] = img;
		img += bytesInRow;
	}
	
	png_write_image(pngStruct, rows);
	png_write_end(pngStruct, NULL);
	png_destroy_write_struct(&pngStruct, &pngInfo);

	fclose(fp);
	delete[] rows;
	return 0;
}

string SaveSlots::getDisplayFitString(const char* str, int limit, float font_size)
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

void SaveSlots::cleanUp()
{
	// Remove empty save folder.
	FileExplorer fileExp;
	fileExp.readDirContent(m_path.c_str());
	vector<DirEntry> dir_content = fileExp.getDirContent();
	if (dir_content.empty()){
		fileExp.rmDir(m_path.c_str());
	}
}

char* SaveSlots::readFileToBuf(const char* file, long* file_size)
{
	// Important: This function allocates memory from heap. Caller must deallocate (delete[]) after use.
	FILE *fp = fopen(file, "r");
	
	if (fp == NULL)
		return NULL;

	// Get the file size in bytes
	fseek(fp, 0L, SEEK_END);
	long numbytes = ftell(fp);
 
	char* buffer = new char[numbytes];
	memset(buffer, 0, numbytes);
	// Reset the file position indicator to the beginning of the file
	fseek(fp, 0L, SEEK_SET);	
	// Copy all the text into the buffer
	fread(buffer, sizeof(char), numbytes, fp);
	fclose(fp);

	*file_size = numbytes;

	return buffer;
}

int SaveSlots::applyPatchModuleSettings(const char* snapshot)
{
	char* settings; 
	
	if (getSettingsFromSnap(snapshot, &settings) < 0)
		return -1;

	string keymaps;
	char* value;
	char* token = strtok(settings, SNAP_MOD_DELIM_ENTRY);

	while (token) {
		
		if (value = strchr(token, '^')){
			*value++ = '\0'; // Now token has the key value
			if (!strcmp(token, INI_FILE_KEY_KEYMAPS)){
				keymaps = value;
			}
			else{
				m_settings->setKeyValue(token, value,0,0,0,1);
			}
		}	
		
		token = strtok(NULL, SNAP_MOD_DELIM_ENTRY);
	}

	// We do this last because loadMappingsFromBuf() also uses strtok.
	if (!keymaps.empty()){
		m_controls->loadMappingsFromBuf(keymaps.c_str());
	}

	// Inform settings of new content
	m_settings->settingsLoaded();

	// View related settings.
	m_settings->applySettings(SETTINGS_VIEW); 
	// Model related settings but missing in the vanilla snap. 
	m_settings->applySettings(SETTINGS_MODEL_NOT_IN_SNAP); 

	delete[] settings;
	return 0;
}

