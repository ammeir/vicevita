
/* view.cpp: GUI implementation and game view rendering.

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

#include "view.h"
#include "controller.h"
#include "control_pad.h"
#include "save_slots.h"
#include "controls.h"
#include "settings.h"
#include "peripherals.h"
#include "menu.h"
#include "about.h"
#include "statusbar.h"
#include "vkeyboard.h"
#include "ini_parser.h"
#include "texter.h"
#include "guitools.h"
#include "iRenderable.h"
#include "resources.h"
#include "stockfont.h"
#include "app_defs.h"
#include "debug_psv.h"

#include <string.h> // memcpy
#include <pthread.h>
#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/power.h>
#include <psp2/kernel/threadmgr.h> 
#include <psp2/io/dirent.h> 

// Globals (Static keyword makes the variable global only inside the file).
string				g_game_file;
vita2d_texture**	g_instructionBitmaps;
static int			gs_instructionBitmapsSize = 0;
static const char*  gs_browserFilter[] = {
	"CRT",												// Cartridge image
	"D64","D71","D80","D81","D82","G64","G41","X64",	// Disk image
	"T64","TAP",										// Tape image
	"PRG","P00",										// Program image
	"ZIP",												// Archive file
	NULL
};

#define VIEW_ASPECT_RATIO_16_9		1
#define VIEW_ASPECT_RATIO_4_3		2
#define VIEW_ASPECT_RATIO_4_3_MAX	3
#define TEXTURE_FILTER_POINT		4
#define TEXTURE_FILTER_LINEAR		5

// Converts RGB triple to an pixel value of format 5-6-5.
#define RGB(r,g,b) ((r>>3)<<11) | ((g>>2)<< 5) | (b>>3)


View::View()
{
	m_controlPad		= NULL;
	m_mainMenu			= NULL;
	m_saveSlots			= NULL;
	m_controls			= NULL;
	m_settings			= NULL;
	m_peripherals		= NULL;
	m_about				= NULL;
	m_statusbar			= NULL;
	m_keyboard			= NULL;
	m_view_tex			= NULL;
	m_fileExp			= NULL;
	m_posX				= 0;
	m_posY				= 0;
	m_scaleX			= 1;
	m_scaleY			= 1;
	m_statusbarMask		= 0;
	m_showStatusbar		= false;
	m_inGame			= false;
	m_pendingDraw		= false;
	m_displayPause		= false;
}

View::~View()
{
	if (m_controlPad)
		delete m_controlPad;
	if (m_mainMenu)
		delete m_mainMenu;
	if (m_saveSlots)
		delete m_saveSlots;
	if (m_controls)
		delete m_controls;
	if (m_settings)
		delete m_settings;
	if (m_peripherals)
		delete m_peripherals;
	if (m_about)
		delete m_about;
	if (m_statusbar)
		delete m_statusbar;
	if (m_keyboard)
		delete m_keyboard;
	if (m_fileExp)
		delete m_fileExp;
	if (m_view_tex)
		vita2d_free_texture(m_view_tex);

	for (int i=0; i<gs_instructionBitmapsSize; ++i){
		vita2d_free_texture(g_instructionBitmaps[i]);
	}

	txtr_free();
}

void View::init(Controller* controller)
{
	//PSV_DEBUG("View::init()");

	m_controller	= controller;
	m_controlPad	= new ControlPad();
	m_mainMenu		= new MainMenu();
	m_saveSlots		= new SaveSlots();
	m_controls		= new Controls();
	m_settings		= new Settings();
	m_peripherals	= new Peripherals();
	m_about			= new About();
	m_statusbar		= new Statusbar();
	m_keyboard		= new VirtualKeyboard;
	m_fileExp		= new FileExplorer;

	createAppDirs();
	createDefConfFile();
	cleanTmpDir();

	vita2d_init();
	
	m_mainMenu->init(this);
	m_settings->init(this, m_controller);
	m_peripherals->init(this, m_controller);
	m_controls->init(m_controller, m_settings);
	m_saveSlots->init(this, m_controller, m_controls, m_settings, 1, 35, 959, 464);
	m_about->init();
	m_statusbar->init(this);
	m_controlPad->init(this, m_controls, m_keyboard);
	m_keyboard->init(this, m_controls);

	loadResources();

	string last_dir = getLastBrowserDir();
	m_fileExp->init(last_dir.c_str(),0,0,0,gs_browserFilter);

	// This is silly but it helps performance.
	// First time saving to ini file is considerably slower than subsequent save. We might as well do it 
	// here so user won't feel any slowdowns.
	IniParser::setValueToIni(DEF_CONF_FILE_PATH, INI_FILE_SEC_FILE_BROWSER, INI_FILE_KEY_LASTDIR, last_dir.c_str(), true);
}

void View::doModal()
{
	string selection;

	m_uiActive = true;
	m_inGame = false;

	while(m_uiActive){
		selection = showMainMenu();
		handleMainMenuSelection(selection);	
	}
}

void View::handleMainMenuSelection(string& selection)
{
	static int load_count = 0;

	if (!selection.compare("Start game")){
		showStartGame();
	}
	else if(!selection.compare("Load/Save")){
		showSaveSlots();
	}
	else if (!selection.compare("Controls")){
		showControls();
	}
	else if (!selection.compare("Settings")){
		showSettings();
	}
	else if (!selection.compare("Devices")){
		showPeripherals();
	}
	else if (!selection.compare("About")){
		showAbout();
	}
	else if (!selection.compare("Reset")){
		m_controller->resetComputer();
		m_inGame = true;
		m_uiActive = false;
	}
	else if (!selection.compare("Resume game")){
		if (g_game_file.empty()){
			g_game_file = "BASIC";
			updateControls();
			updateSettings();
		}
		m_inGame = true;
		m_uiActive = false;
		waitKeysIdle(); // To avoid pushing circle button inside game
		
		// Vice only draws when necessary and if a game is in a still image a draw won't come.
		// Draw the last buffer we got so we atleast respond to the button press. 
		updateView(); 
	}
}

int	View::createView(int width, int height, int bpp)
{
	// Creates the view texture.

	m_width = width;
	m_height = height;
	
	if (m_view_tex)
		vita2d_free_texture(m_view_tex);


	switch (bpp){
	case 8:
		// format: 8 bit indexed
		m_view_tex = vita2d_create_empty_texture_format(m_width, m_height, (SceGxmTextureFormat)SCE_GXM_TEXTURE_BASE_FORMAT_P8);
		m_viewBitDepth = 8;
		break;
	case 16:
		// format: 16 bit 5-6-5
		m_view_tex = vita2d_create_empty_texture_format(m_width, m_height, SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB);
		m_viewBitDepth = 16;
		break;
	}

	m_settings->applySetting(TEXTURE_FILTER);
    m_view_tex_data = (unsigned char*) vita2d_texture_get_datap(m_view_tex);
	
	return 1;
}

void View::updateView()
{
	if (!m_inGame)
		return;

	vita2d_start_drawing();
	vita2d_clear_screen();

	// Don't draw view if keyboard is in fullscreen.
	//if (!(g_keyboardStatus == KEYBOARD_UP && m_keyboard->getMode() == KEYBOARD_FULL_SCREEN)){
		vita2d_draw_texture_part_scale(
			m_view_tex, 
			m_posX, 
			m_posY, 
			m_viewport.x, 
			m_viewport.y, 
			m_viewport.width, 
			m_viewport.height, 
			m_scaleX, 
			m_scaleY);
	//}

	if (g_keyboardStatus & KEYBOARD_VISIBLE)
		m_keyboard->render();

	if (m_showStatusbar)
		m_statusbar->render();

	if (m_displayPause)
		txtr_draw_text(870, 534, YELLOW, "Paused");
	
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void View::updateViewPos()
{
	//PSV_DEBUG("View::updateViewPos()");

	if (g_keyboardStatus == KEYBOARD_UP && m_keyboard->getMode() != KEYBOARD_FULL_SCREEN){
		// Split screen keyboard view. 
		
		// Hide the borders to get a bigger screen.
		m_viewport.x = 136;
		m_viewport.y = 51;
		m_viewport.width = 320;
		m_viewport.height = 200;

		float scaleY = (float)276/m_viewport.height; 
		float scaleX = (float)16/9;
		m_posX = (960 - 320*scaleX) / 2;
		m_posY = 0;
		m_scaleX = (float)16/9;
		m_scaleY = scaleY;

		return;
	}
	
	switch (m_aspectRatio){
	case VIEW_ASPECT_RATIO_16_9:
		m_posX = 0;
		m_posY = 0;
		m_scaleX = (float)960/m_viewport.width;
		m_scaleY = (!m_showStatusbar)? (float)544/m_viewport.height: (float)514/m_viewport.height;
		break;
	case VIEW_ASPECT_RATIO_4_3:
		m_posX = (960-m_viewport.width*2)/2;
		m_posY = (544-m_viewport.height*2)/2;
		m_scaleX = 2;
		m_scaleY = (!m_showStatusbar)? 2: (float)514/272;	
		break;
	case VIEW_ASPECT_RATIO_4_3_MAX:
		m_scaleX = (float)544/m_viewport.height;
		m_scaleY = (!m_showStatusbar)? m_scaleX: (float)514/m_viewport.height;
		m_posX = (960-(m_viewport.width*m_scaleX))/2;
		m_posY = 0;
		break;
	default:
		break;
	};
}

void View::updateViewport(int x, int y, int width, int height)
{
	//PSV_DEBUG("View::updateViewport()");
	m_viewport.x = x;
	m_viewport.y = y;
	m_viewport.width = width;
	m_viewport.height = height;

	updateViewPos();
}

void View::getViewInfo(int* width, int* height, unsigned char** ppixels, int* pitch, int* bpp)
{
	if (width)
		*width = m_width;
	if (height)
		*height = m_height;
	if (ppixels)
		*ppixels = m_view_tex_data;
	if (pitch)
		*pitch = m_width * (m_viewBitDepth/8);
	if (bpp)
		*bpp = m_viewBitDepth;
}

void View::getViewportInfo(int* x, int* y, int* width, int* height)
{
	if (x) *x = m_viewport.x;
	if (y) *y = m_viewport.y;
	if (width) *width = m_viewport.width;
	if (height) *height = m_viewport.height;
}

void View::setPalette(unsigned char* palette, int size)
{
	// Fills the color palette table of the indexed texture

	if (!m_view_tex)
		return;

	uint32_t* palette_tbl = (uint32_t*)vita2d_texture_get_palette(m_view_tex);

	if (!palette_tbl)
		return;

	unsigned char r, g, b;
	
	for(int i=0; i<size; i++){
		r = palette[0];
		g = palette[1];
		b = palette[2];
		palette_tbl[i] = r | (g << 8) | (b << 16) | (0xFF << 24);
		palette += 3;
	}
}

void View::setFPSCount(int fps, int percent, int warp_flag)
{
	m_statusbar->setSpeedData(fps, percent, warp_flag);
}

void View::setTapeCounter(int counter)
{
	m_statusbar->setTapeCounter(counter);
}

void View::setTapeControl(int control)
{
	m_statusbar->setTapeControl(control);
}

void View::setDriveLed(int drive, int led)
{
	m_statusbar->setDriveLed(drive, led);
}

void View::setTapeMotorStatus(int motor)
{
	m_statusbar->setTapeMotor(motor);
}

void View::setDriveTrack(unsigned int drive, unsigned int track)
{
	m_statusbar->setDriveTrack(drive, track);
}

void View::setDriveDiskPresence(int drive, int disk_in)
{
	m_statusbar->setDriveDiskPresence(drive, disk_in);
}

void View::setDriveStatus(int drive, int active)
{
	m_statusbar->setDriveStatus(drive, active);
}

int View::showMessage(const char* msg, int msg_type)
{
	int ret = 0;
	if (msg_type == 0)
		gtShowMsgBoxOk(msg);
	else
		ret = gtShowMsgBoxOkCancel(msg);

	return ret;
}

void View::displayPaused(int val)
{
	m_displayPause = val? true: false;
}

void View::toggleStatusbarOnView()
{
	m_showStatusbar = !m_showStatusbar;
	updateViewPos();

	if (!m_showStatusbar)
		m_pendingDraw = true; // Force view update incase we are in still image.
}

void View::toggleKeyboardOnView()
{
	m_keyboard->toggleVisibility();
	m_keyboard->clear();
}

string	View::getGameSaveDirPath()
{
	string game_file = g_game_file;
	// Get the file name
	size_t slash_pos = g_game_file.find_last_of("/");
	if (slash_pos != string::npos)
		game_file = g_game_file.substr(slash_pos+1, string::npos);
	// Remove extension
	size_t dot_pos = game_file.find_last_of(".");
	if (dot_pos != string::npos)
		game_file = game_file.substr(0, dot_pos);

	return SAVE_DIR + game_file + "/";
}

 bool View::fileExist(string& file_name) 
 {
    if (FILE *file = fopen(file_name.c_str(), "r")) {
        fclose(file);
        return true;
    } 

    return false; 
}

void View::createAppDirs()
{
	string dirs[9];
	dirs[0] = APP_DATA_DIR;
	dirs[1] = GAME_DIR;
	dirs[2] = SAVE_DIR;
	dirs[3] = VICE_DIR;
	dirs[4] = TMP_DIR;
	dirs[5] = TMP_DRV8_DIR;
	dirs[6] = TMP_DRV9_DIR;
	dirs[7] = TMP_DRV10_DIR;
	dirs[8] = TMP_DRV11_DIR;

	for (int i=0; i<9; ++i){
		if (!m_fileExp->dirExist(dirs[i].c_str()))
			m_fileExp->makeDir(dirs[i].c_str());
	}
}

void View::createDefConfFile()
{
	m_settings->createConfFile(DEF_CONF_FILE_PATH);
}

bool View::isBorderlessView()
{
	return m_settings->getKeyValue(BORDERS) == "Hide"? true: false;
}

void View::scanControls(ControlPadMap** maps, int* size, bool scan_mouse)
{
	m_controlPad->scan(maps, size, g_keyboardStatus == KEYBOARD_UP, scan_mouse);
}

string View::showMainMenu()
{
	m_mainMenu->doModal();
	return m_mainMenu->getSelection();
}

void View::showStartGame()
{

	// Check if browser directory needs update.
	string last_dir = getLastBrowserDir();
	if (last_dir != m_fileExp->getDir())
		m_fileExp->init(last_dir.c_str(),0,0,0,gs_browserFilter);

	string entry_dir = m_fileExp->getDir();
	string selection;
	int ret;
	
	do{
		selection = m_fileExp->doModal();
		if (selection.empty()) 
			break;
		gtShowMsgBoxNoBtn("Loading...", m_fileExp);
		ret = m_peripherals->loadImage(CTRL_AUTO_DETECT_LOAD, selection.c_str());
		if (ret < 0)
			gtShowMsgBoxOk("Could not start image", m_fileExp);
	}
	while(ret);

	string exit_dir = m_fileExp->getDir();

	// Save directory if it has changed.
	if (exit_dir != entry_dir)
		IniParser::setValueToIni(DEF_CONF_FILE_PATH, INI_FILE_SEC_FILE_BROWSER, INI_FILE_KEY_LASTDIR, exit_dir.c_str());

	if (!selection.empty()){
		updateControls();
		updateSettings();
		m_inGame = true;
		m_uiActive = false;
	}
}

void View::showSaveSlots()
{
	string save_dir = (g_game_file.empty())? "": getGameSaveDirPath();
	string file_name = getFileNameNoExt(g_game_file.c_str());

	if (m_saveSlots->doModal(save_dir.c_str(), file_name.c_str()) == EXIT_MENU){
		m_inGame = true;
		m_uiActive = false;
	}
}

void View::showPeripherals()
{
	RetCode ret = m_peripherals->doModal();

	if (ret == EXIT_MENU){
		// User pressed Auto load.
		m_inGame = true;
		m_uiActive = false;
		updateControls();
		updateSettings();
		// Vice only draws when necessary and if a game is in a still image a draw won't come.
		// Draw the last buffer we got so we atleast respond to the button press. 
		updateView(); 
	}
}

void View::showControls()
{
	string save_dir = (g_game_file.empty())? "": getGameSaveDirPath();
	string file_name = getFileNameNoExt(g_game_file.c_str());
	m_controls->doModal(save_dir.c_str(), file_name.c_str());
}

void View::showSettings()
{
	string save_dir = (g_game_file.empty())? "": getGameSaveDirPath();
	string file_name = getFileNameNoExt(g_game_file.c_str());

	m_settings->doModal(save_dir.c_str(), file_name.c_str());
}

void View::showAbout()
{
	m_about->doModal();
}

void View::updateControls()
{
	// Load key mappings from a configuration file.

	if (!m_controls)
		return;
	
	// Get game specific config file.
	string conf_file  = getGameSaveDirPath() + CONF_FILE_NAME;

	// If file or the value is missing, get the default config file.
	if (!fileExist(conf_file) || !m_controls->mappingsExistInFile(conf_file.c_str()))
		conf_file = DEF_CONF_FILE_PATH;

	m_controls->loadMappingsFromFile(conf_file.c_str());
}

void View::updateSettings()
{
	// Load settings from a configuration file.

	if (!m_settings)
		return;
	
	// Get game specific config file.
	string conf_file  = getGameSaveDirPath() + CONF_FILE_NAME;

	// If file or the value is missing, get the default config file.
	if (!fileExist(conf_file) || !m_settings->settingsPopulatedInFile(conf_file.c_str()))
		conf_file = DEF_CONF_FILE_PATH;

	m_settings->loadSettingsFromFile(conf_file.c_str());
	// Inform settings of new content.
	m_settings->settingsLoaded();
	m_settings->applySettings(SETTINGS_ALL);
}

void View::changeAspectRatio(const char* value)
{
	if (!strcmp(value, "16:9")){
		m_aspectRatio = VIEW_ASPECT_RATIO_16_9;
	}
	else if (!strcmp(value, "4:3")){
		m_aspectRatio = VIEW_ASPECT_RATIO_4_3;
	}
	else if (!strcmp(value, "4:3 max")){ // Scaled to maximum height
		m_aspectRatio = VIEW_ASPECT_RATIO_4_3_MAX;
	}
}

void View::changeKeyboardMode(const char* value)
{
	int mode;
	
	if (!strcmp(value, "Slider"))
		mode = KEYBOARD_SLIDER;
	else if (!strcmp(value, "Split screen"))
		mode = KEYBOARD_SPLIT_SCREEN;
	else if (!strcmp(value, "Full screen"))
		mode = KEYBOARD_FULL_SCREEN;
	else
		return;

	m_keyboard->setMode(mode);

	// Calculate new keyboard positon.
	switch (mode){
	case KEYBOARD_FULL_SCREEN:
	{
		float scaleX = (float)960/868;
		float scaleY = 1.44;
		int y = (544-265*scaleY)/2;
		int x = 0;
		m_keyboard->setPosition(x, y, scaleX, scaleY);
		break;
	}
	case KEYBOARD_SLIDER:
	case KEYBOARD_SPLIT_SCREEN:
		m_keyboard->setPosition((960-868) / 2, 278, 1, 1);
		break;
	default:
		break;
	};
}

void View::changeTextureFilter(const char* value)
{
	if (!strcmp(value, "Linear"))
		vita2d_texture_set_filters(m_view_tex, (SceGxmTextureFilter)SCE_GXM_TEXTURE_FILTER_LINEAR, (SceGxmTextureFilter)SCE_GXM_TEXTURE_FILTER_LINEAR);
	else if (!strcmp(value, "Point"))
		vita2d_texture_set_filters(m_view_tex, (SceGxmTextureFilter)SCE_GXM_TEXTURE_FILTER_POINT, (SceGxmTextureFilter)SCE_GXM_TEXTURE_FILTER_POINT);
}

void View::setHostCpuFrequency(const char* freq)
{
	// Change vita cpu clock frequency

	if (!strcmp(freq, "333 MHz")){
		scePowerSetArmClockFrequency(333);
		scePowerSetGpuClockFrequency(166);
		scePowerSetBusClockFrequency(166);
		scePowerSetGpuXbarClockFrequency(111);
	}
	else if (!strcmp(freq, "444 MHz")){
		scePowerSetArmClockFrequency(444);
		scePowerSetGpuClockFrequency(222);
		scePowerSetBusClockFrequency(222);
		scePowerSetGpuXbarClockFrequency(166);
	}
}

void View::changeJoystickScanSide(const char* side)
{
	m_controlPad->changeJoystickScanSide(side);
}

void View::waitKeysIdle()
{
	m_controlPad->waitTillButtonsReleased();
}

void View::onSettingChanged(int key, const char* value, const char* src, const char** values, int size, int mask)
{
	// Model setting changed.

	switch (key){
	case DRIVE:
	case DRIVE_STATUS:
	case DRIVE_TRUE_EMULATION:
	case DRIVE_SOUND_EMULATION:
	case DATASETTE:
	case DATASETTE_RESET_WITH_CPU:
	case DATASETTE_CONTROL:
	case CARTRIDGE:
	case CARTRIDGE_RESET:
		m_peripherals->setKeyValue(key, value, src, values, size, mask);
		break;
	default:
		m_settings->setKeyValue(key, value, src, values, size, mask);
	}
}

void View::applySetting(int key_id)
{
	m_settings->applySetting(key_id);
}

void View::applyAllSettings()
{
	m_settings->applySettings(SETTINGS_ALL);
	m_peripherals->applyAllSettings();
}

void View::setProperty(int key, const char* value)
{
	switch (key){
	case ASPECT_RATIO:
		changeAspectRatio(value);
		updateViewPos();
		break;
	case TEXTURE_FILTER:
		changeTextureFilter(value);
		break;
	case BORDERS:
		m_controller->setBorderVisibility(value);
		break;
	case JOYSTICK_SIDE:
		changeJoystickScanSide(value);
		break;
	case JOYSTICK_AUTOFIRE_SPEED:
		m_controller->setJoystickAutofireSpeed(value);
		break;
	case KEYBOARD_MODE:
		changeKeyboardMode(value);
		break;
	case HOST_CPU_SPEED:
		setHostCpuFrequency(value);
		break;
	}
}

void View::getSettingValues(int key, const char** value, const char** src, const char*** values, int* size)
{
	switch (key){
	case DRIVE:
	case DRIVE_NUMBER:
	case DRIVE_STATUS:
	case DRIVE_TRUE_EMULATION:
	case DRIVE_SOUND_EMULATION:
	case DATASETTE:
	case CARTRIDGE:
	case CARTRIDGE_RESET:
		m_peripherals->getKeyValues(key, value, src, values, size);
		break;
	default:
		m_settings->getKeyValues(key, value, src, values, size);
	}
}

void View::activateMenu()
{
	doModal();
}

int View::convertRGBToPixel(uint8_t red, uint8_t green, uint8_t blue)
{
	// Converts RGB triple to a pixel value of format 5-6-5.
	return RGB(red, green, blue);
}

bool View::pendingRedraw()
{
	// Return true if the keyboard was pressed or statusbar was changed.
	
	if (m_pendingDraw){
		m_pendingDraw = false;
		return true;
	}

	if (m_keyboard->isUpdated())
		return true;
	
	if (m_showStatusbar && m_statusbar->isUpdated())
		return true;

	return false;
}

unsigned char* View::getThumbnail()
{
	uint32_t* palette_tbl = (uint32_t*)vita2d_texture_get_palette(m_view_tex);
	
	if (!palette_tbl)
		return NULL;

	static ViewPort vp;

	// Remove the borders
	if (m_controller->getViewport(&vp, false) < 0)
		return NULL;
	
	unsigned char* bitmap = new unsigned char[vp.width * vp.height * 3];
	unsigned char* p = bitmap;
	unsigned char* pixel = m_view_tex_data + (vp.y * m_width + vp.x);

	for (int i = 0; i < vp.height; i++)
    {
		for (int j = 0; j < vp.width; j++, pixel++)
		{
			//palette table value: r | (g << 8) | (b << 16) | (0xFF << 24);
			p[0] = palette_tbl[*pixel]; // red
			p[1] = (palette_tbl[*pixel] >> 8); // green
			p[2] = (palette_tbl[*pixel] >> 16); // blue
			p += 3;
		}

		pixel += (m_width - vp.width);
    } 

	return bitmap;
}

void View::notifyReset()
{
	// Notification that Model is about to reset.

	//m_peripherals->notifyReset();
	m_statusbar->notifyReset();
	updateControls();
	updateSettings();
}
	
string View::getFileNameNoExt(const char* fpath)
{
	string fname = fpath;
	
	// Remove path.
	size_t slash_pos = fname.find_last_of("/");
	if (slash_pos != string::npos){
		fname = fname.substr(slash_pos+1, string::npos);
	}else{
		size_t colon_pos = fname.find_last_of(":");
		if (colon_pos != string::npos){
			fname = fname.substr(colon_pos+1, string::npos);
		}
	}

	// Remove extension.
	size_t comma_pos = fname.find_last_of(".");
	if (comma_pos != string::npos){
		fname = fname.substr(0, comma_pos);
	}

	return fname;
}

void View::cleanTmpDir()
{
	m_fileExp->readDirContent(TMP_DIR);
	vector<DirEntry> dir_content = m_fileExp->getDirContent();

	for (vector<DirEntry>::iterator it = dir_content.begin(); it != dir_content.end(); ++it){
		m_fileExp->deleteFile((*it).path.c_str());
	}

	m_fileExp->readDirContent(TMP_DRV8_DIR);
	dir_content = m_fileExp->getDirContent();

	for (vector<DirEntry>::iterator it = dir_content.begin(); it != dir_content.end(); ++it){
		m_fileExp->deleteFile((*it).path.c_str());
	}

	m_fileExp->readDirContent(TMP_DRV9_DIR);
	dir_content = m_fileExp->getDirContent();

	for (vector<DirEntry>::iterator it = dir_content.begin(); it != dir_content.end(); ++it){
		m_fileExp->deleteFile((*it).path.c_str());
	}

	m_fileExp->readDirContent(TMP_DRV10_DIR);
	dir_content = m_fileExp->getDirContent();

	for (vector<DirEntry>::iterator it = dir_content.begin(); it != dir_content.end(); ++it){
		m_fileExp->deleteFile((*it).path.c_str());
	}

	m_fileExp->readDirContent(TMP_DRV11_DIR);
	dir_content = m_fileExp->getDirContent();

	for (vector<DirEntry>::iterator it = dir_content.begin(); it != dir_content.end(); ++it){
		m_fileExp->deleteFile((*it).path.c_str());
	}
}

string	View::getLastBrowserDir()
{
	// Get last saved directory. If not found use default game directory.
	string ret = GAME_DIR;
	const char* key_value = NULL;

	int err_code = IniParser::getValueFromIni(DEF_CONF_FILE_PATH, INI_FILE_SEC_FILE_BROWSER, INI_FILE_KEY_LASTDIR, &key_value);
	
	if (err_code == INI_PARSER_OK && key_value && m_fileExp->dirExist(key_value)){
		ret = key_value;
		delete[] key_value;
	}

	return ret;
}

void View::loadResources()
{
	gs_instructionBitmapsSize = 16;
	g_instructionBitmaps = new vita2d_texture*[gs_instructionBitmapsSize];
	g_instructionBitmaps[0] = vita2d_load_PNG_buffer(img_btn_navigate_up_down);
	g_instructionBitmaps[1] = vita2d_load_PNG_buffer(img_btn_navigate_up_down_left);
	g_instructionBitmaps[2] = vita2d_load_PNG_buffer(img_btn_navigate_up_down_x);
	g_instructionBitmaps[3] = vita2d_load_PNG_buffer(img_btn_dpad_left_blue);
	g_instructionBitmaps[4] = vita2d_load_PNG_buffer(img_btn_triangle_red);
	g_instructionBitmaps[5] = vita2d_load_PNG_buffer(img_btn_triangle_magenta);
	g_instructionBitmaps[6] = vita2d_load_PNG_buffer(img_btn_circle_green);
	g_instructionBitmaps[7] = vita2d_load_PNG_buffer(img_btn_circle_yellow);
	g_instructionBitmaps[8] = vita2d_load_PNG_buffer(img_btn_cross_green);
	g_instructionBitmaps[9] = vita2d_load_PNG_buffer(img_btn_square_magenta);
	g_instructionBitmaps[10] = vita2d_load_PNG_buffer(img_btn_ltrigger_blue);
	g_instructionBitmaps[11] = vita2d_load_PNG_buffer(img_btn_rtrigger_blue);
	g_instructionBitmaps[12] = vita2d_load_PNG_buffer(img_btn_circle_blue);
	g_instructionBitmaps[13] = vita2d_load_PNG_buffer(img_btn_cross_blue);
	g_instructionBitmaps[14] = vita2d_load_PNG_buffer(img_btn_square_blue);
	g_instructionBitmaps[15] = vita2d_load_PNG_buffer(img_btn_triangle_blue);
}


