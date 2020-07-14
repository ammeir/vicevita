
/* controller.cpp: Purpose of the controller is to act as a middle man 
				   between View and Model (VICE).

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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#ifndef __cplusplus
// for gcc
void		PSV_CreateView(int width, int height, int depth);
void		PSV_UpdateView();
void		PSV_SetViewport(int x, int y, int width, int height);
void		PSV_GetViewInfo(int* width, int* height, unsigned char** ppixels, int* pitch, int* bpp);
void		PSV_ScanControls();
void		PSV_ApplySettings();
void		PSV_ActivateMenu();
int			PSV_RGBToPixel(uint8_t r, uint8_t g, uint8_t b);
void		PSV_NotifyPalette(unsigned char* palette, int size);
void		PSV_NotifyFPS(int fps, float percent, int warp_flag);
void		PSV_NotifyTapeCounter(int count);
void		PSV_NotifyTapeControl(int control);
void		PSV_NotifyDriveStatus(int drive, int led);
void		PSV_NotifyDriveContent(int drive, const char* image);
void		PSV_NotifyTapeMotorStatus(int motor);
void		PSV_NotifyDriveTrack(unsigned int drive, unsigned int track);
void		PSV_NotifyReset();
int			PSV_ShowMessage(const char* msg, int msg_type);
#else

#include "view.h"
#include <string>
#include <vector>

extern "C" {
#include "imagecontents.h"
}

extern int g_joystickPort;

typedef struct{
	string fname;
	string fpath;
}file_info_s;

typedef struct{
	int row;
	int column;
	int ispress;
}key_action_s;

typedef struct{
	const char* snapshot_file;
	const char* module_name;
	uint8_t		major;
	uint8_t		minor;
	const char*	data;
	uint32_t	data_size;
}patch_data_s;

enum ctrl_load_e{
	CTRL_AUTO_DETECT_LOAD,
	CTRL_DISK_LOAD,
	CTRL_TAPE_LOAD,
	CTRL_CART_LOAD
};

using std::string;
using std::vector;

class View;
class Peripherals;
struct dev_data_s;
class Controller
{

private:
	bool			m_inGame;

	string			getFileNameFromPath(const char* fpath);
	string			getFileNameNoExt(const char* fpath);
	void			changeJoystickPort(const char* port);
	void			setCpuSpeed(const char* val);
	void			setAudioPlayback(const char* val);
	void			setSidEngine(const char* val);
	void			setSidModel(const char* val);
	void			setViciiModel(const char* val);
	void			setCrtEmulation();
	void			setColorPalette(const char* val);
	void			setJoystickPort(const char* val);
	void			setDriveStatus(const char* val);
	void			setDriveEmulation(const char* val);
	void			setDriveSoundEmulation(const char* val);
	void			setDatasetteReset(const char* val);
	void			setCartridgeReset(const char* val);
	void			setMachineResetMode(const char* val);
	void			setMouseSampling(const char* val);
	int				getImageType(const char* image);
	void			getImageFileContents(int peripheral, const char* image, const char*** values, int* size);
	void			updatePalette();
	void			resumeSound();
	int				attachDriveImage(int drive, const char* image);
	int				attachTapeImage(const char* image);
	int				attachCartridgeImage(const char* image);
	void			detachDriveImage(int drive);
	int				detachTapeImage();
	void			detachCartridgeImage();

public:
					Controller();
					~Controller();

	void			init(View* view);
	void			resetComputer();
	int				loadFile(int load_type, const char* file_path, int index = 0);
	int				loadState(const char* file);
	int				saveState(const char* file_name);
	int				patchSaveState(patch_data_s* patch);
	int				getSaveStatePatch(patch_data_s* patch_info);
	int				getSaveStatePatchInfo(patch_data_s* patch_info);
	int				getViewport(ViewPort* vp, bool borders);
	void			syncSetting(int key);
	void			syncPeripherals();
	void			syncModelSettings();
	void			setModelProperty(int key, const char* value);
	int				attachImage(int device, const char* image);
	void			detachImage(int device);
	void			setTapeControl(int action);
	void			setCartControl(int action);
	void			setBorderVisibility(const char* val);
	void			setJoystickAutofireSpeed(const char* val);
};


#endif
#endif