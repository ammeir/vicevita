
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

enum KeyboardMode{KEYBOARD_FULL_SCREEN = 0, KEYBOARD_SPLIT_SCREEN};
enum AspectRatio {VIEW_ASPECT_RATIO_16_9 = 0, VIEW_ASPECT_RATIO_4_3, VIEW_ASPECT_RATIO_4_3_MAX};
enum TextureFilter {TEXTURE_FILTER_POINT = 0, TEXTURE_FILTER_LINEAR};
enum RetCode {EXIT, EXIT_MENU};


typedef struct{
	int x;
	int y;
	int width;
	int height;
} ViewPort;

extern string			g_game_file;
extern KeyboardMode		g_keyboardMode;
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
class vita2d_texture;
class IRenderable;
struct FileInfo;
class View
{

private:

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
	
	AspectRatio		m_aspectRatio;
	TextureFilter	m_textureFilter;

	vita2d_texture*	m_view_tex;
	unsigned char*	m_view_tex_data;
	int				m_width;
	int				m_height;
	ViewPort		m_viewport;
	int				m_viewSizeInBytes;
	int				m_viewBitDepth;
	bool			m_keyboardOnView;
	bool			m_uiActive;
	bool			m_inGame;
	int				m_statusbarMask;
	
	// x,y positions of different views
	float			m_posXNormalView;
	float			m_posYNormalView;
	float			m_posXSplitView;
	float			m_posYSplitView;
	// x and y scales of different views
	float			m_scaleXNormalView;
	float			m_scaleYNormalView;
	float			m_scaleXSplitView;
	float			m_scaleYSplitView;

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
	void			updateSettings();
	void			showSpeedStats();
	void			showDatasetteStats();
	void			loadResources();
	void			changeAspectRatio(AspectRatio value);
	void			changeTextureFilter(TextureFilter value);
	void			changeKeyboardMode(KeyboardMode value);
	void			changeJoystickScanSide(const char* side);
	void			waitKeysIdle();
	AspectRatio		strToAspectRatio(const char* value);
	TextureFilter	strToTextureFilter(const char* value);
	KeyboardMode	strToKeyboardMode(const char* value);
	string			getFileNameFromPath(const char* fpath);
	void			setHostCpuFrequency(const char* freq);
	void			cleanTmpDir();
	void			printTestRect();


public: 
					View();
					~View();
	void			init(Controller* controller);

	void			doModal();
	void			scanControls(char* joy_pins, ControlPadMap** maps, int* size, bool scan_mouse);
	int				createView(int width, int height, int bpp);
	void			updateView();
	void			updateViewport(int x, int y, int width, int height);
	void			getViewInfo(int* width, int* height, unsigned char** ppixels, int* pitch, int* bpp);
	void			getViewportInfo(int* x, int* y, int* width, int* height);
	void			setPalette(unsigned char* palette, int size);
	void			setFPSCount(int fps, int percent, int warp_flag);
	void			setTapeCounter(int count);
	void			setTapeControl(int status);
	int				showMessage(const char* msg, int msg_type);
	void			displayPaused(int);
	void			toggleKeyboardOnView();
	bool			isKeyboardOnView();
	bool			isBorderlessView();
	void			applyAllSettings();
	void			setProperty(int key, const char* value);
	void			activateMenu();
	unsigned char*	getThumbnail();
	void			notifyReset();
	void			onSettingChanged(int key, const char* value, const char* value2, const char** values, int size, int mask);
	void			getSettingValues(int key, const char** value, const char** value2, const char*** values, int* size);
	int				convertRGBToPixel(uint8_t red, uint8_t green, uint8_t blue);
	bool			pendingRedraw();
};


#endif