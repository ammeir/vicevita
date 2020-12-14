
/* view.h: GUI implementation and game view rendering.

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

#ifndef VIEW_H
#define VIEW_H

#include "vkeyboard.h"
#include <string>
#include <psp2/types.h>


using std::string;

enum RetCode {EXIT, EXIT_MENU};

typedef struct{
	int x;
	int y;
	int width;
	int height;
} ViewPort;

extern string			g_game_file;
extern vita2d_texture** g_instructionBitmaps;


class Controller;
class ControlPad;
class MainMenu;
class SaveSlots;
class Peripherals;
class Controls;
class Settings;
class About;
class Statusbar;
class FileExplorer;
class vita2d_texture;
class IRenderable;
struct FileInfo;
class View
{

private:

	float			m_posX;
	float			m_posY;
	float			m_scaleX;
	float			m_scaleY;
	ViewPort		m_viewport;
	vita2d_texture*	m_view_tex;
	unsigned char*	m_view_tex_data;

	Controller*		m_controller;
	ControlPad*		m_controlPad;
	MainMenu*		m_mainMenu;
	SaveSlots*		m_saveSlots;
	Peripherals*	m_peripherals;
	Controls*		m_controls;
	Settings*		m_settings;
	VirtualKeyboard* m_keyboard;
	About*			m_about;
	Statusbar*		m_statusbar;
	FileExplorer*	m_fileExp;
	int				m_aspectRatio;
	int				m_width;
	int				m_height;
	int				m_viewSizeInBytes;
	int				m_viewBitDepth;
	bool			m_uiActive;
	bool			m_inGame;
	int				m_showStatusbar;
	bool			m_statusbarMask;
	bool			m_displayPause;
	bool			m_pendingDraw;
	
	string			showMainMenu();
	void			showStartGame();
	void			showSaveSlots();
	void			showPeripherals();
	void			showControls();
	void			showSettings();
	void			showAbout();
	
	void			handleExitButton(unsigned int exit_button);
	void			handleMainMenuSelection(string& selection);
	string			selectGame();
	string			getGameSaveDirPath();
	bool			fileExist(string& file_name);
	void			createAppDirs();
	void			createDefConfFile();
	void			renderKeyboard();
	void			showSpeedStats();
	void			showDatasetteStats();
	void			loadResources();
	void			changeAspectRatio(const char* value);
	void			changeKeyboardMode(const char* value);
	void			changeTextureFilter(const char* value);
	void			changeJoystickScanSide(const char* side);
	void			waitKeysIdle();
	string			getFileNameNoExt(const char* fpath);
	void			setHostCpuFrequency(const char* freq);
	void			updateControls();
	void			updateSettings();
	void			cleanTmpDir();
	string			getLastBrowserDir();
	void			printTestRect();

public: 
					View();
					~View();
	void			init(Controller* controller);

	void			doModal();
	void			scanControls(ControlPadMap** maps, int* size, bool scan_mouse);
	int				createView(int width, int height, int bpp);
	void			updateView();
	void			updateViewPos();
	void			updateViewport(int x, int y, int width, int height);
	void			getViewInfo(int* width, int* height, unsigned char** ppixels, int* pitch, int* bpp);
	void			getViewportInfo(int* x, int* y, int* width, int* height);
	void			setPalette(unsigned char* palette, int size);
	void			setFPSCount(int fps, int percent, int warp_flag);
	void			setTapeCounter(int count);
	void			setTapeControl(int status);
	void			setDriveLed(int drive, int led);
	void			setTapeMotorStatus(int motor);
	void			setDriveTrack(unsigned int drive, unsigned int track);
	void			setDriveDiskPresence(int drive, int disk_in);
	void			setDriveStatus(int drive, int active);
	int				showMessage(const char* msg, int msg_type);
	void			displayPaused(int);
	void			toggleStatusbarOnView();
	void			toggleKeyboardOnView();
	bool			isBorderlessView();
	void			applySetting(int);
	void			applyAllSettings();
	void			setProperty(int key, const char* value);
	void			activateMenu();
	unsigned char*	getThumbnail();
	void			notifyReset();
	void			onSettingChanged(int key, const char* value, const char* src, const char** values, int size, int mask);
	void			getSettingValues(int key, const char** value, const char** src, const char*** values, int* size);
	int				convertRGBToPixel(uint8_t red, uint8_t green, uint8_t blue);
	bool			pendingRedraw();
};


#endif