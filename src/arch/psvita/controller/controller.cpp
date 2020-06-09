
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

#include "controller.h"
#include "control_pad.h"
#include "file_explorer.h"
#include "unzip.h"
#include "guitools.h"
#include "ctrl_defs.h"
#include "app_defs.h"
#include "debug_psv.h"
#include "resid/sid.h"

// C interface files. Tell g++ not to mangle symbols.
extern "C" {
#include "main.h"
#include "ui.h"
#include "autostart.h"
#include "cartridge.h"
#include "resources.h"
#include "mem.h"
#include "video.h"
#include "machine.h"
#include "sound.h"
#include "vsync.h"
#include "sid.h"
#include "joy.h"
#include "attach.h"
#include "tape.h"
#include "datasette.h"
#include "c64cartsystem.h"
#include "joystick.h"
#include "keyboard.h"
#include "log.h"
#include "videoarch.h"
#include "tapecontents.h"
#include "diskcontents.h"
#include "attach.h"
#include "lib.h"
#include "mousedrv.h"
#include "raster.h"
#include "snapshot.h"
#include "kbdbuf.h"
#include "maincpu.h"
#include "t64.h"
}

#include <cstring>
#include <stdio.h>
#include <pthread.h>
#include <psp2/kernel/threadmgr.h> 


static View* gs_view;

extern "C" int PSV_CreateView(int width, int height, int depth)
{
	return gs_view->createView(width, height, depth);
}

extern "C" void PSV_UpdateView()
{
	gs_view->updateView();
	// Avoid updating the view twice.
	gs_frameDrawn = true;
}

extern "C" void PSV_SetViewport(int x, int y, int width, int height)
{
	// Make sure we stay borderless when e.g. changing video standard.
	if (gs_view->isBorderlessView()){
		video_canvas_s* canvas;
		video_psv_get_canvas(&canvas);
		width = canvas->geometry->gfx_size.width;
		height = canvas->geometry->gfx_size.height;
		x = canvas->geometry->extra_offscreen_border_left + canvas->geometry->gfx_position.x;
		y = canvas->geometry->gfx_position.y;
	}

	gs_view->updateViewport(x, y, width, height);
}

extern "C" void	PSV_GetViewInfo(int* width, int* height, unsigned char** ppixels, int* pitch, int* bpp)
{
	gs_view->getViewInfo(width, height, ppixels, pitch, bpp);
}

extern "C" void PSV_ScanControls()
{
	static ControlPadMap* maps[16];
	int size = 0;


	// Goto main menu at boot time. There must be a better place to implement this.
	if (gs_bootTime){
		video_psv_menu_show();
		gs_bootTime = false;
		return;
	}

	gs_view->scanControls(maps, &size, gs_scanMouse);

	for (int i=0; i<size; ++i){
		ControlPadMap* map = maps[i];
		
		if (!map) continue;
		if (map->isjoystick){
			if (map->ispress)
				joystick_value[g_joystickPort] |= map->joypin;
			else
				joystick_value[g_joystickPort] &= ~map->joypin;
			continue;
		}
		if (map->iskey){
			// Get row and column from the mid. If bit 4 is set, negate the row value.
			int row = (map->mid & 0x08)? -(map->mid >> 4): (map->mid >> 4);
			int column = map->mid & 0x07;
			keyboard_set_keyarr_any(row, column, map->ispress);
			continue;
		}

		// Button releases can be ignored for all special commands except autofire.
		if (!map->ispress && map->mid != 136) 
			continue;

		// Special commands
		switch (map->mid){
		case 126: // Show menu
			if (ui_emulation_is_paused()){
				// We can show menu right away. No need to trigger trap if in pause state
				// and the sound buffer is already silenced.
				gs_view->activateMenu();
				setSoundVolume(100); 
				break;
			}

			setPendingAction(CTRL_ACTION_SHOW_MENU);
			break;
		case 127: // Show/hide keyboard
			gs_view->toggleKeyboardOnView();
			keyboard_clear_keymatrix(); // Remove any keys that were not released.
			gs_view->updateView(); // Force draw in case of still image.
			break;
		case 128: // Pause/unpause computer
			if (!ui_emulation_is_paused())
				setPendingAction(CTRL_ACTION_PAUSE);
			else{
				ui_pause_emulation(0);
				gs_view->displayPaused(0);
				gs_view->updateView();
				setSoundVolume(100); 
			}
			break;
		case 129: // Swap joysticks
			toggleJoystickPorts();
			break;
		case 130: // Toggle warp mode
			toggleWarpMode();
			break;
		case 136: // Turn autofire on/off
			if (map->ispress)
				gs_autofireOn = true;
			else{
				gs_autofireOn = false;
				joystick_value[g_joystickPort] &= ~0x10; 
			}
			break;
		case 137: // Reset computer
			if (!ui_emulation_is_paused()){ // Reseting in pause state causes freeze.
				machine_trigger_reset(gs_machineResetMode);
				gs_view->notifyReset();
			}
			break;
		case 138: // Show/hide status bar
			gs_view->toggleStatusbarOnView();
			break;
		default:
			break;
		}
	}

	if (gs_autofireOn){
		// Toggle joystick fire.
		if (++gs_frameCounter % gs_moduloDivider == 0){
			uint8_t joy_pins = joystick_value[g_joystickPort];
			joystick_value[g_joystickPort] = (joy_pins & 0x10)? joy_pins & ~0x10: joy_pins | 0x10;
			gs_frameCounter = 0;
		}
	}

	checkPendingActions();

	// Because Vice updates the screen inconsistently, we have a problem with updating the statusbar and
	// showing keyboard magnifying boxes. This seems out of place here but as the scan happens after the 
	// end of each frame it's a pretty good place to constantly check for something.
	// gs_frameDrawn variable is used to avoid updating the view twice on one frame.
	// It works because screen draw is allways followed by the scan.
	if (!gs_frameDrawn && gs_view->pendingRedraw())
		gs_view->updateView();

	gs_frameDrawn = false;
}

extern "C" void PSV_ApplySettings()
{
	// Disable CRT emulation.
	resources_set_int(VICE_RES_VICII_FILTER, 0); 

	// Enable general mechanisms for fast disk/tape emulation.
	resources_set_int(VICE_RES_VIRTUAL_DEVICES, 1);

	// This is very important for ReSID performance.
	// Any other value brings the emulation to almost complete standstill.
	resources_set_int(VICE_RES_SID_RESID_SAMPLING,0);

	// Apply all user defined settings.
	gs_view->applyAllSettings();
}

extern "C" void PSV_ActivateMenu()
{
	gs_view->activateMenu();
}

extern "C" int PSV_RGBToPixel(uint8_t r, uint8_t g, uint8_t b)
{
	return gs_view->convertRGBToPixel(r,g,b);
}

extern "C" void PSV_NotifyPalette(unsigned char* palette, int size)
{
	gs_view->setPalette(palette, size);
}

extern "C" void PSV_NotifyFPS(int fps, float percent, int warp_flag)
{
	gs_view->setFPSCount(fps, (int)percent, warp_flag);
}

extern "C" void	PSV_NotifyTapeCounter(int counter)
{
	gs_view->setTapeCounter(counter);
}

extern "C" void	PSV_NotifyTapeControl(int control)
{
	gs_view->setTapeControl(control);
}

extern "C" void	PSV_NotifyDriveStatus(int drive, int led)
{
	gs_view->setDriveStatus(drive, led);
}

extern "C" void	PSV_NotifyDriveContent(int drive, const char* image)
{
	//gs_view->notifyDriveContent(drive, image);
}

extern "C" void PSV_NotifyTapeMotorStatus(int motor)
{
	gs_view->setTapeMotorStatus(motor);
}

extern "C" int	PSV_ShowMessage(const char* msg, int msg_type)
{
	return gs_view->showMessage(msg, msg_type);
}

extern "C" void	PSV_NotifyReset()
{
	gs_view->notifyReset();
}

Controller::Controller()
{

}

Controller::~Controller()
{
}

void Controller::init(View* view)
{
	gs_view = view;
	for (int i=0; i<4; ++i) m_zipFileSlots.push_back("");
}

int Controller::loadFile(load_type_e load_type, const char* file, const char* program_name, int index, const char* target_file)
{
	//PSV_DEBUG("Controller::loadFile");
	//PSV_DEBUG("Image: %s", file);
	//PSV_DEBUG("Program name: %s", program_name);
	//PSV_DEBUG("index: %d", index);
	

	if (!file)
		return -1;

	int ret = 0;

	switch (load_type){
	case AUTO_DETECT_LOAD:
		{

		file = extractFile(file);

		if (!file)
			return -1;

		int device = getImageType(file);

		// Remove any attached cartridge or it will be loaded instead.
		cartridge_detach_image(-1);
		// This is not necessary but we also remove any attached disk or tape.
		detachTapeImage();
		detachDriveImage();

		// Sometimes a tape won't run without reseting the datasette.
		datasette_control(DATASETTE_CONTROL_RESET);
	
		// This prevents sound loss when loading game when previous load hasn't finished.
		resources_set_int(VICE_RES_WARP_MODE, 0); 

		// Unpause emulation.
		pauseEmulation(false);

		// Cartridge won't load if 'CartridgeReset' is disabled. Enable it temporarily.
		int cartridge_reset = 1;
		if (device == IMAGE_CARTRIDGE){
			resources_get_int(VICE_RES_CARTRIDGE_RESET, &cartridge_reset);
			if (!cartridge_reset)
				resources_set_int(VICE_RES_CARTRIDGE_RESET, 1);
		}
	
		gtShowMsgBoxNoBtn("Loading...");

		ret = autostart_autodetect(file, NULL, index, AUTOSTART_MODE_RUN);

		// Restore old value.
		if (!cartridge_reset)
			resources_set_int(VICE_RES_CARTRIDGE_RESET, 0);

		break;
		}
	case CART_LOAD:
		if (cartridge_attach_image(CARTRIDGE_CRT, file) < 0)
			return -1;
		//g_game_file = file;
		pauseEmulation(false);
		break;
	case DISK_LOAD:
		{
		// This prevents sound loss when loading game when previous load hasn't finished.
		resources_set_int(VICE_RES_WARP_MODE, 0); 

		char* prog_name = NULL;
		string str_prog_name;

		image_contents_t *contents = diskcontents_filesystem_read(file);
		if (contents) {
			prog_name = image_contents_filename_by_number(contents, index);
			image_contents_destroy(contents);
		}

		// Remove 0xa0 characters from file names.
		if (prog_name){
			str_prog_name = prog_name;
			size_t pos = str_prog_name.find_first_of(0xa0);
			if (pos != string::npos){
				str_prog_name = str_prog_name.substr(0, pos);
			}
			lib_free(prog_name);
		}

		gs_loadProgramName = !str_prog_name.empty()? str_prog_name.c_str(): "*";
		gs_loadImageName = file;
		setPendingAction(CTRL_ACTION_LOAD_DISK);
		pauseEmulation(false);
		break;
		}
	case TAPE_LOAD:
		{
		if (!tape_image_dev1)
			return -1;
      
		char* prog_name = NULL;
		string str_prog_name;

	
		if (tape_image_dev1->type == TAPE_TYPE_TAP){
			// TAP files.
			tape_seek_to_file(tape_image_dev1, index);
		}
		else{
			// T64 files. 
			
			// Find the name of the program file. 
			
			//image_contents_s* contents = tapecontents_read(file);
			//if (contents) {
			//	prog_name = image_contents_filename_by_number(contents, index+1); // Indexes start at 1.
			//	image_contents_destroy(contents);
			//}

			//// Remove 0xa0 characters from file names.
			//if (prog_name){
			//	str_prog_name = prog_name;
			//	size_t pos = str_prog_name.find_first_of(0xa0);
			//	if (pos != string::npos){
			//		str_prog_name = str_prog_name.substr(0, pos);
			//	}
			//	lib_free(prog_name);
			//}

			if (index > 0){
				tape_seek_to_file(tape_image_dev1, index-1);
			}
			else{
				// There seems to be a bug when trying to seek to the first file in a t64 archive. 
				// Using index 0 won't give you the first file but the second.
				// The workaround here is to use the 'total available file records' - 1, 
				// which will result to VICE not finding the file and rewinding to the first record.
				// Not sure why this happens. This was just a trial and error discovery.
				int size = ((t64_t*)(tape_image_dev1->data))->header.num_entries;
				tape_seek_to_file(tape_image_dev1, size-1);
			}
		}
	
		gs_loadProgramName = str_prog_name.c_str();
		gs_loadImageName = file;
		setPendingAction(CTRL_ACTION_LOAD_TAPE);
		pauseEmulation(false);
		break;
		}
	}

	return ret;
}

int Controller::loadState(const char* file)
{
	// Prevent sound loss when loading a state when previous load hasn't finished.
	resources_set_int(VICE_RES_WARP_MODE, 0);
	
	//char* sfile = lib_stralloc(file);
	//ui_load_snapshot(sfile);
	//return 1;

	string cartridge;
	const char* file_name = cartridge_get_file_name(cart_getid_slotmain());
	
	if (file_name)
		cartridge = file_name;

	int ret = machine_read_snapshot((char*)file, 0);
	
	if (!cartridge.empty()){
		// When loading a snapshot Vice detaches any attached cartridges. Reading the slot returns null 
		// but when doing a reset it still loads the old cartridge. Not sure if this is a bug or my missunderstanding. 
		// The workaround is to read the cart name before loading the snapshot and then attaching it back after. 
		// CartridgeReset has to be disabled or otherwise the cpu will reset.
		int cartridge_reset;
		resources_get_int(VICE_RES_CARTRIDGE_RESET, &cartridge_reset);
		resources_set_int(VICE_RES_CARTRIDGE_RESET, 0);
		cartridge_attach_image(CARTRIDGE_CRT, cartridge.c_str());
		resources_set_int(VICE_RES_CARTRIDGE_RESET, cartridge_reset);
	}

	return ret;

	//return machine_read_snapshot((char*)file, 0);	
}

int Controller::saveState(const char* file_name)
{
	return machine_write_snapshot(file_name, 0, 0, 0);
}

int Controller::patchSaveState(patch_data_s* patch)
{
	if (!patch || !patch->snapshot_file || !patch->data)
		return -1;
	
	FILE *fp;

	if (!(fp = fopen(patch->snapshot_file, "a+"))){
		return -1;
	}

	// We can't add a module to a snapshot without changing code in Vice.
	// Workaround is to append a byte chunk to the snapshot file that is in right format.
	// Vice will happily accept it.
	
	// Vice snapshot module format:
	// Header:                 
	// Name (16 bytes)        
	// Major version (1 byte) 
	// Minor version (1 byte1)	  
	// Size of module (4 bytes)   
	// Data: (Any number of fields. We use two, dword and byte array)
	// Data size (4 bytes)		 
	// Data  (n bytes)           

	int module_size = 16 + 1 + 1 + 4 + 4 + patch->data_size;
	char* buf = new char[module_size];

	char name[16] = {0};

	if (patch->module_name)
		strncpy(name, patch->module_name, 16);

	// Header
	memcpy(buf, name, 16);
	buf[16] = patch->major;
	buf[17] = patch->minor;
	buf[18] = module_size;
	buf[19] = module_size >> 8;
	buf[20] = module_size >> 16;
	buf[21] = module_size >> 24;
	// Data
	buf[22] = patch->data_size;
	buf[23] = patch->data_size >> 8;
	buf[24] = patch->data_size >> 16;
	buf[25] = patch->data_size >> 24;
	memcpy(&buf[26], patch->data, patch->data_size);
	
	if (fwrite(buf, 1, module_size, fp) < 1) {
		fclose(fp);
        return -1;
    }

	fclose(fp);
	return 0;
}

int	Controller::getSaveStatePatch(patch_data_s* pinfo)
{
	uint8_t major_version;
	uint8_t minor_version;
	snapshot_t* snapshot;
	snapshot_module_t* patch_module;

	// Reading our patch module is best done by using the snapshot api.
	// This way we don't have to remember the file offset to our data.

	// First we need to open the snapshot.
	snapshot = snapshot_open(pinfo->snapshot_file, &major_version, &minor_version, machine_get_name());
	
	if (!snapshot){
		return -1;
	}

	// Get our patch module from the snapshot.
	patch_module = snapshot_module_open(snapshot,
                                        pinfo->module_name,
                                        &pinfo->major,
                                        &pinfo->minor);

	if (!patch_module){
		snapshot_close(snapshot);
		return -1;
	}

	// Read the byte array size.
	if (snapshot_module_read_dword(patch_module, &pinfo->data_size) < 0){
		snapshot_module_close(patch_module);
		snapshot_close(snapshot);
		return -1;
	}

	// Read the byte array.
	snapshot_module_read_byte_array(patch_module,
                                    (uint8_t*)pinfo->data, 
									pinfo->data_size);
	

	snapshot_module_close(patch_module);
	snapshot_close(snapshot);

	return 0;
}

int	Controller::getSaveStatePatchInfo(patch_data_s* pinfo)
{
	uint8_t major_version;
	uint8_t minor_version;
	snapshot_t* snapshot;
	snapshot_module_t* patch_module;

	snapshot = snapshot_open(pinfo->snapshot_file, &major_version, &minor_version, machine_get_name());
	
	if (!snapshot){
		return -1;
	}
	
	patch_module = snapshot_module_open(snapshot,pinfo->module_name,&pinfo->major,&pinfo->minor);
	
	if (!patch_module){
		snapshot_close(snapshot);
		return -1;
	}
		
	// Read the byte array size.
	if (snapshot_module_read_dword(patch_module, &pinfo->data_size) < 0){
		snapshot_module_close(patch_module);
		snapshot_close(snapshot);
		return -1;
	}
	
	snapshot_module_close(patch_module);
	snapshot_close(snapshot);

	return 0;
}

void Controller::resetComputer()
{
	machine_trigger_reset(gs_machineResetMode);
}

void Controller::setModelProperty(int key, const char* value)
{
	if (!key || !value)
		return;

	switch (key){
	case JOYSTICK_PORT:
		changeJoystickPort(value);break;
	case COLOR_PALETTE:
		setColorPalette(value);break;
	case CPU_SPEED:
		setCpuSpeed(value);break;
	case SOUND:
		setAudioPlayback(value);break;
	case SID_ENGINE:
		setSidEngine(value);break;
	case SID_MODEL:
		setSidModel(value);break;
	case VICII_MODEL:
		setViciiModel(value);break;
	case DRIVE_TRUE_EMULATION:
		setDriveEmulation(value);break;
	case DRIVE_SOUND_EMULATION:
		setDriveSoundEmulation(value);break;
	case DATASETTE_RESET_WITH_CPU:
		setDatasetteReset(value);break;
	case CARTRIDGE_RESET:
		setCartridgeReset(value);break;
	case MACHINE_RESET:
		setMachineResetMode(value); break;
	}
}

void Controller::getImageFileContents(int peripheral, const char* image, const char*** values, int* values_size)
{
	*values = NULL;
	*values_size = 0;
	image_contents_t* content = NULL;


	if (peripheral == DRIVE8 || peripheral == DATASETTE){
		// Retrieve disk/tape contents

		if (peripheral == DRIVE8)
			content = diskcontents_read(image, 8);
		else if (peripheral == DATASETTE)
			content = tapecontents_read(image);

		if (!content)
			return;

		image_contents_file_list_t* entry = content->file_list;

		// Get list size;
		int size = 0;
		while (entry){
			size++;
			entry = entry->next;
		}

		if (!size)
			return;
		
		char** p = new char*[size];
		*values = (const char**)p;
		*values_size = size;
		entry = content->file_list;
		for (int i=0; i<size; ++i){
			*p = image_contents_file_to_string(entry, 1); // allocates memory from heap.
			entry = entry->next;
			p++;
		}
	}

	if (content){
		image_contents_destroy(content);
	}
}

void Controller::changeJoystickPort(const char* port)
{
	if (!strcmp(port, "Port 1")){
		g_joystickPort = 1;
		resources_set_int(VICE_RES_JOY_PORT1_DEV, 1);  // 1 = Joystick
		resources_set_int(VICE_RES_JOY_PORT2_DEV, 0);  // 0 = None
	}
	else if (!strcmp(port, "Port 2")){
		g_joystickPort = 2;
		resources_set_int(VICE_RES_JOY_PORT2_DEV, 1);
		resources_set_int(VICE_RES_JOY_PORT1_DEV, 0);
	}
}

void Controller::setCpuSpeed(const char* val)
{
	int value;

	if(!strcmp(val, "100%"))
		value = 100;
	else if (!strcmp(val, "125%"))
		value = 125;
	else if (!strcmp(val, "150%"))
		value = 150;
	else if (!strcmp(val, "175%"))
		value = 175;
	else if (!strcmp(val, "200%"))
		value = 200;
	else 
		value = 100;

	resources_set_int(VICE_RES_CPU_SPEED, value);
}

void Controller::setAudioPlayback(const char* val)
{
	int value = !strcmp(val, "Enabled")? 1: 0; 
	resources_set_int(VICE_RES_SOUND, value);
}

void Controller::setSidEngine(const char* val)
{
	int value;
	if (!strcmp(val, "FastSID"))
		value = SID_ENGINE_FASTSID;
	else if (!strcmp(val, "ReSID"))
		value = SID_ENGINE_RESID;
	else
		return;

	resources_set_int(VICE_RES_SID_ENGINE, value);
	
}

void Controller::setSidModel(const char* val)
{
	int value;
	if (!strcmp(val, "6581"))
		value = SID_MODEL_6581;
	else if (!strcmp(val, "8580"))
		value = SID_MODEL_8580;
	else
		return;

	resources_set_int(VICE_RES_SID_MODEL, value);
}

void Controller::setBorderVisibility(const char* val)
{
	// We show/hide borders by changing the viewport size. This is much more user friendly than
	// changing the VICIIBorderMode resource which will result in a computer reset.

	int x,y,width,height;
	struct video_canvas_s* canvas;

	video_psv_get_canvas(&canvas);

	if (!strcmp(val,"Hide")){
		width = canvas->geometry->gfx_size.width;
		height = canvas->geometry->gfx_size.height;
		x = canvas->geometry->extra_offscreen_border_left + canvas->geometry->gfx_position.x;
		y = canvas->geometry->gfx_position.y;
	}
	else{
		width = canvas->draw_buffer->canvas_width;
		height = canvas->draw_buffer->canvas_height;
		x = canvas->geometry->extra_offscreen_border_left;
		y = canvas->geometry->first_displayed_line;
	}

	gs_view->updateViewport(x,y,width,height);
}

/* 
Removing borders by changing the viewport has one problem. It sometimes shows visual artifacts on
screen edges. This happens seldomly and can be prevented by changing the texture filtering value to 'Point'.
This version can also remove the borders by changing the VICIIBorderMode resource.
Commented out because lack of testing. 
*/
//void Controller::setBorderVisibility(const char* val)
//{
//	int value;
//  if (resources_get_int("VICIIBorderMode", &value) < 0)
//		return;
//
//	int x,y,width,height;
//	struct video_canvas_s* canvas;
//	
//	video_psv_get_canvas(&canvas);
//
//	if (value == 0/*VICII_NORMAL_BORDERS*/){
//		if (!strcmp(val,"Hide")){
//			width = canvas->geometry->gfx_size.width;// - 2;
//			height = canvas->geometry->gfx_size.height;// - 2;
//			x = canvas->geometry->extra_offscreen_border_left + canvas->geometry->gfx_position.x;// + 1;
//			y = canvas->geometry->gfx_position.y;//  + 1;
//		}
//		else if (!strcmp(val,"Show")){
//			width = canvas->draw_buffer->canvas_width;
//			height = canvas->draw_buffer->canvas_height;
//			x = canvas->geometry->extra_offscreen_border_left;
//			y = canvas->geometry->first_displayed_line;
//		}
//		else if (!strcmp(val,"Remove")){
//			resources_set_int("VICIIBorderMode", 3/*VICII_NO_BORDERS*/);
//			return;
//		}
//
//		gs_view->updateViewport(x,y,width,height);
//	}
//	else if (value == 3/*VICII_NO_BORDERS*/){
//		if (!strcmp(val,"Show")){
//			resources_set_int("VICIIBorderMode", 0/*VICII_NORMAL_BORDERS*/);	
//		}
//	}	
//}

void Controller::setJoystickAutofireSpeed(const char* val)
{
	// We toggle joystick fire by using modulo function: frame counter % divider.
	// It's maybe not the best approach but it's the easiest.
	if (!strcmp(val, "Fast"))
		gs_moduloDivider = 2; // PAL: 12 clicks/s  NTSC 15 clicks/s
	else if (!strcmp(val, "Medium fast"))
		gs_moduloDivider = 3; // PAL: 8 clicks/s  NTSC 10 clicks/s
	else if (!strcmp(val, "Medium"))
		gs_moduloDivider = 6; // PAL: 4 clicks/s  NTSC 5 clicks/s
	else if (!strcmp(val, "Medium slow"))
		gs_moduloDivider = 12; // PAL: 2 clicks/s  NTSC 2 clicks/s
	else if (!strcmp(val, "Slow"))
		gs_moduloDivider = 25; // PAL: 1 click/s  NTSC 1 click/s

	gs_frameCounter = 0;
}

void Controller::setViciiModel(const char* val)
{
	int value = MACHINE_SYNC_PAL;

	if (!strcmp(val, "NTSC"))
		value = MACHINE_SYNC_NTSC;
	else if (!strcmp(val, "Old NTSC"))
		value = MACHINE_SYNC_NTSCOLD;
	else if (!strcmp(val, "PAL-N"))
		value = MACHINE_SYNC_PALN;

	resources_set_int(VICE_RES_MACHINE_VIDEO_STANDARD, value); 
}

void Controller::setCrtEmulation()
{
	resources_set_int(VICE_RES_VICII_DOUBLE_SCAN, 1);
	resources_set_int(VICE_RES_VICII_DOUBLE_SIZE, 1);
	// Crt emulation (scan lines) 0=off, 1=on
	resources_set_int(VICE_RES_VICII_FILTER, 1); 
}

void Controller::setColorPalette(const char* val)
{
	const char* curr_palette;
	const char* new_palette = NULL;

    resources_get_string_sprintf("%sPaletteFile", &curr_palette, "VICII");
	if (!curr_palette) curr_palette = "";

	if (!strcmp(val, "Pepto (PAL)") && strcmp("pepto-pal", curr_palette))
		new_palette = "pepto-pal";
	else if (!strcmp(val, "Colodore") && strcmp("colodore", curr_palette))
		new_palette = "colodore";
	else if (!strcmp(val, "Vice") && strcmp("vice", curr_palette))
		new_palette = "vice";
	else if (!strcmp(val, "Ptoing") && strcmp("ptoing", curr_palette))
		new_palette = "ptoing";
	else if (!strcmp(val, "RGB") && strcmp("rgb", curr_palette))
		new_palette = "rgb";
	else if (!strcmp(val, "None"))
		new_palette = "None";
	
	if (new_palette){
		if (strcmp(val, "None")){
			resources_set_int(VICE_RES_VICII_EXTERNAL_PALETTE, 1);
			resources_set_string_sprintf("%sPaletteFile", new_palette, "VICII");
		}
		else{
			resources_set_int(VICE_RES_VICII_EXTERNAL_PALETTE, 0);
		}

		updatePalette();
	}
}

void Controller::setDriveEmulation(const char* val)
{
	// Enable/Disable hardware-level emulation of disk drives.
	if (!strcmp(val, "Fast"))
		resources_set_int(VICE_RES_DRIVE_TRUE_EMULATION, 0);
	else if (!strcmp(val, "True"))
		resources_set_int(VICE_RES_DRIVE_TRUE_EMULATION, 1);
}

void Controller::setDriveSoundEmulation(const char* val)
{
	// Enable/Disable sound emulation of disk drives.
	if (!strcmp(val, "Disabled"))
		resources_set_int(VICE_RES_DRIVE_SOUND_EMULATION, 0);
	else if (!strcmp(val, "Enabled"))
		resources_set_int(VICE_RES_DRIVE_SOUND_EMULATION, 1);
}

void Controller::setJoystickPort(const char* val)
{
	if (!strcmp(val, "Port 1")){
		resources_set_int(VICE_RES_JOY_DEVICE_1, JOYDEV_JOYSTICK);
		resources_set_int(VICE_RES_JOY_DEVICE_2, JOYDEV_NONE);
	}
	else if (!strcmp(val, "Port 2")){
		resources_set_int(VICE_RES_JOY_DEVICE_1, JOYDEV_NONE);
		resources_set_int(VICE_RES_JOY_DEVICE_2, JOYDEV_JOYSTICK);
	}
}

void Controller::setDatasetteReset(const char* val)
{
	// Enable/Disable automatic Datasette-Reset.
	if (!strcmp(val, "Enabled"))
		resources_set_int(VICE_RES_DATASETTE_RESET_WITH_CPU, 1);
	else if (!strcmp(val, "Disabled"))
		resources_set_int(VICE_RES_DATASETTE_RESET_WITH_CPU, 0);
}

void Controller::setCartridgeReset(const char* val)
{
	// Reset machine if a cartridge is attached or detached
	if (!strcmp(val, "Enabled"))
		resources_set_int(VICE_RES_CARTRIDGE_RESET, 1);
	else if (!strcmp(val, "Disabled"))
		resources_set_int(VICE_RES_CARTRIDGE_RESET, 0);
}

void Controller::setMachineResetMode(const char* val)
{
	if (!strcmp(val, "Hard"))
		gs_machineResetMode = MACHINE_RESET_MODE_HARD;
	else if (!strcmp(val, "Soft"))
		gs_machineResetMode = MACHINE_RESET_MODE_SOFT;
}

int Controller::attachDriveImage(const char* val)
{
	// Currently only drive 8 is supported
	if(!strcmp(val, "Empty"))
		return -1;

	if (file_system_attach_disk(8, val) < 0)
		return -1;

	return 0;
}

int Controller::attachTapeImage(const char* val)
{
	if(!strcmp(val, "Empty"))
		return -1;

	if (tape_image_attach(1, val) < 0)
		return -1;

	return 0;
}

int Controller::attachCartridgeImage(const char* val)
{
	if(!strcmp(val, "Empty"))
		return -1; 

	if (cartridge_attach_image(CARTRIDGE_CRT, val) < 0)
		return -1;

	return 0;
}

void Controller::detachDriveImage()
{
	file_system_detach_disk(8);
}

int Controller::detachTapeImage()
{
	return tape_image_detach(1);
}

void Controller::detachCartridgeImage()
{
	cartridge_detach_image(-1);

	// Detaching cartridge will result in a hard reset if 'CartridgeReset' is set.
	int cartridge_reset;
	resources_get_int(VICE_RES_CARTRIDGE_RESET, &cartridge_reset);
	if (cartridge_reset)
		gs_view->notifyReset();
}

void Controller::syncSetting(int key)
{
	// Sync setting with VICE.

	//string value;
	const char* image_file = NULL;
	const char* key_value;
	const char* key_value2;
	const char** value_list;
	int list_size = 0;


	switch (key){
	case DRIVE8:
		// Check if drive has image attached.
		image_file = file_system_get_disk_name(8);

		// Get the listbox values in settings
		gs_view->getSettingValues(key, &key_value, &key_value2, &value_list, &list_size);

		if (!image_file){
			// No disk image attached. 
			// Deallocate old listbox values if any.
			if (value_list && list_size){
				const char** p = value_list;
				for (int i=0; i<list_size; ++i){
					lib_free(*p++);
				}
				delete[] value_list;
			}

			gs_view->onSettingChanged(key,"Empty","",0,0,15);
		}
		else{
			if (!strcmp(key_value, "Empty")){
				// Image is attached to device but is not showing in settings.
				// Get drive contents and populate listbox.
				getImageFileContents(key, image_file, &value_list, &list_size);
				if (list_size){
					gs_view->onSettingChanged(key, value_list[0], image_file, value_list, list_size, 15);
				}
			}
			else{
				// Device has attachement and something is showing in settings.
				// Do nothing if image is current.
				if (!strcmp(key_value2, image_file)){
					return;
				}
				// New disk image attached. First deallocate old list values.
				if (value_list && list_size){
					const char** p = value_list;
					for (int i=0; i<list_size; ++i){
						lib_free(*p++);
					}
					delete[] value_list;
				}

				getImageFileContents(key, image_file, &value_list, &list_size);
				if (list_size){
					gs_view->onSettingChanged(key, value_list[0], image_file, value_list, list_size, 15);
				}	
			}
		}
		break;
	case DRIVE_TRUE_EMULATION:
		{
		int val; 
		string str_val;
		
		if (resources_get_int(VICE_RES_DRIVE_TRUE_EMULATION, &val) < 0)
			return;
		
		str_val = val? "True": "Fast";
		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	case DRIVE_SOUND_EMULATION:
		{
		int val; 
		string str_val;
		
		if (resources_get_int(VICE_RES_DRIVE_SOUND_EMULATION, &val) < 0)
			return;
		
		str_val = val? "Enabled": "Disabled";
		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	case DATASETTE:
		{
		image_file = tape_get_file_name();

		// Get the listbox values in settings
		gs_view->getSettingValues(key, &key_value, &key_value2, &value_list, &list_size);

		if (!image_file){
			// No tape image attached. Deallocate old listbox values if any.
			if (list_size > 0){
				const char** p = value_list;
				for (int i=0; i<list_size; ++i){
					lib_free(*p++);
				}
				delete[] value_list;
			}

			gs_view->onSettingChanged(key,"Empty","",0,0,15);
			return;
		}

		if (!strcmp(key_value2, image_file)){
			// Same tape image attached. Do nothing.
			return;
		}

		// New tape image attached. Delete old list.
		if (list_size > 0){
			const char** p = value_list;
			for (int i=0; i<list_size; ++i){
				lib_free(*p++);
			}
			delete[] value_list;
		}

		string fname = getFileNameFromPath(image_file);
		
		// VICE is painfully slow at retrieving tape content. 
		// To avoid a freezing effect just set the image name as the key value.
		gs_view->onSettingChanged(key, fname.c_str(), image_file,0,0,15);
		break;
		}
	case DATASETTE_RESET_WITH_CPU:
		{
		int val; 
		string str_val;
		
		if (resources_get_int(VICE_RES_DATASETTE_RESET_WITH_CPU, &val) < 0)
			return;
		
		str_val = val? "Enabled": "Disabled";
		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	case CARTRIDGE:
		image_file = cartridge_get_file_name(cart_getid_slotmain());
		if (image_file)
			gs_view->onSettingChanged(key, getFileNameFromPath(image_file).c_str(),image_file,0,0,3);
		else
			gs_view->onSettingChanged(key, "Empty","",0,0,3);
		break;
	case CARTRIDGE_RESET:
		{
		int val; 
		string str_val;
		
		if (resources_get_int(VICE_RES_CARTRIDGE_RESET, &val) < 0)
			return;
		
		str_val = val? "Enabled": "Disabled";
		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	case VICII_MODEL:
		{
		int val; 
		string str_val;

		if (resources_get_int(VICE_RES_MACHINE_VIDEO_STANDARD, &val) < 0)
			return;
		
		switch (val){
		case MACHINE_SYNC_PAL:
			str_val = "PAL";
			break;
		case MACHINE_SYNC_NTSC:
			str_val = "NTSC";
			break;
		case MACHINE_SYNC_NTSCOLD:
			str_val = "Old NTSC";
			break;
		case MACHINE_SYNC_PALN:
			str_val = "PAL-N";
			break;
		}

		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	case SID_ENGINE:
		{
		int val; 
		string str_val;

		if (resources_get_int(VICE_RES_SID_ENGINE, &val) < 0)
			return;
		
		switch (val){
		case SID_ENGINE_FASTSID: str_val = "FastSID"; break;
		case SID_ENGINE_RESID: str_val = "ReSID"; break;
		}
		
		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	case SID_MODEL:
		{
		int val; 
		string str_val;

		if (resources_get_int(VICE_RES_SID_MODEL, &val) < 0)
			return;
		
		switch (val){
		case SID_MODEL_6581: str_val = "6581"; break;
		case SID_MODEL_8580: str_val = "8580"; break;
		}
		
		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	case JOYSTICK_PORT:
		{
		int val_port1;
		int val_port2;
		string str_val;

		// Check what we have attached on the control ports.
		// 0=none, 1=joystick, 2=paddles, 3=mouse(1351) etc.
		if (resources_get_int(VICE_RES_JOY_PORT1_DEV, &val_port1) < 0)
			return;

		if (resources_get_int(VICE_RES_JOY_PORT2_DEV, &val_port2) < 0)
			return;

		if (val_port1 == 1 && val_port2 != 1){
			g_joystickPort = 1;
			str_val = "Port 1";
		}
		else if (val_port2 == 1 && val_port1 != 1){
			g_joystickPort = 2;
			str_val = "Port 2";
		}

		if (!str_val.empty())
			gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);

		break;
		}
	case CPU_SPEED:
		{
		int val; 
		string str_val;

		if (resources_get_int(VICE_RES_CPU_SPEED, &val) < 0)
			return;
		
		switch (val){
		case 100: str_val = "100%"; break;
		case 125: str_val = "125%"; break;
		case 150: str_val = "150%"; break;
		case 175: str_val = "175%"; break;
		case 200: str_val = "200%"; break;
		}

		if (!str_val.empty())
			gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);

		break;
		}
	case SOUND:
		{
		int val; 
		string str_val;
		if (resources_get_int(VICE_RES_SOUND, &val) < 0)
			return;

		str_val = val==1? "Enabled": "Disabled";
		gs_view->onSettingChanged(key, str_val.c_str(),0,0,0,1);
		break;
		}
	default:
		break;
	}
}

void Controller::syncPeripherals()
{
	syncSetting(DRIVE8);
	syncSetting(DRIVE_TRUE_EMULATION);
	syncSetting(DRIVE_SOUND_EMULATION);
	syncSetting(DATASETTE);
	syncSetting(CARTRIDGE);
	syncSetting(CARTRIDGE_RESET);
}

void Controller::syncModelSettings()
{
	syncSetting(VICII_MODEL);
	syncSetting(DRIVE_SOUND_EMULATION);
	syncSetting(SID_ENGINE);
	syncSetting(SID_MODEL);
	syncSetting(COLOR_PALETTE);
	syncSetting(JOYSTICK_PORT);
	syncSetting(CPU_SPEED);
	syncSetting(SOUND);
	syncSetting(DRIVE_TRUE_EMULATION);
	syncSetting(DRIVE_SOUND_EMULATION);
	syncSetting(CARTRIDGE_RESET);
}

void Controller::updatePalette()
{
	video_psv_update_palette();
}

void Controller::resumeSound()
{
	sound_resume();
}

string Controller::getFileNameFromPath(const char* fpath)
{
	string fname = fpath;

	size_t slash_pos = fname.find_last_of("/");
	if (slash_pos != string::npos){
		return fname.substr(slash_pos+1, string::npos);
	}

	size_t colon_pos = fname.find_last_of(":");
	if (colon_pos != string::npos){
		return fname.substr(colon_pos+1, string::npos);
	}

	return fname;
}

int Controller::getImageType(const char* image)
{
	int ret = -1;
	string extension;
	string file = image;

	static const char* disk_ext[] = {"D64","D71","D80","D81","D82","G64","G41","X64",0};
	static const char* tape_ext[] = {"T64","TAP",0};
	static const char* cart_ext[] = {"CRT",0};
	static const char* prog_ext[] = {"PRG","P00",0};

	size_t dot_pos = file.find_last_of(".");
	if (dot_pos != string::npos)
		extension = file.substr(dot_pos+1, string::npos);

	if (extension.empty())
		return ret;
	
	strToUpperCase(extension);
	
	const char** p = disk_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_DISK;
		p++;
	}
	
	p = tape_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_TAPE;
		p++;
	}
	
	p = cart_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_CARTRIDGE;
		p++;
	}
	
	p = prog_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_PROGRAM;
		p++;
	}

	return ret;
}

void Controller::setTapeControl(int action)
{
	switch (action){
	case DATASETTE_CONTROL_STOP:
	case DATASETTE_CONTROL_START:
	case DATASETTE_CONTROL_FORWARD:
	case DATASETTE_CONTROL_REWIND:
	case DATASETTE_CONTROL_RECORD:
	case DATASETTE_CONTROL_RESET:
	case DATASETTE_CONTROL_RESET_COUNTER:
		datasette_control(action);
		break;
	}
}

void Controller::setCartControl(int action)
{
	switch (action){
	case CART_CONTROL_FREEZE:
		keyboard_clear_keymatrix();
        cartridge_trigger_freeze(); 
		break;
	case CART_CONTROL_SET_DEFAULT:
		cartridge_set_default(); 
		break;
	case CART_CONTROL_FLUSH_IMAGE:
		break;
	case CART_CONTROL_SAVE_IMAGE:
		break;
	}
}

void Controller::strToUpperCase(string& str)
{
   for (std::string::iterator p = str.begin(); p != str.end(); ++p)
       *p = toupper(*p);
}

void Controller::getViewport(ViewPort* vp, bool borders)
{
	struct video_canvas_s* canvas;

	video_psv_get_canvas(&canvas);

	if (borders){
		vp->width = canvas->draw_buffer->canvas_width;
		vp->height = canvas->draw_buffer->canvas_height;
		vp->x = canvas->geometry->extra_offscreen_border_left;
		vp->y = canvas->geometry->first_displayed_line;
	}
	else{
		vp->width = canvas->geometry->gfx_size.width;
		vp->height = canvas->geometry->gfx_size.height;
		vp->x = canvas->geometry->extra_offscreen_border_left + canvas->geometry->gfx_position.x;
		vp->y = canvas->geometry->gfx_position.y;
	}
}

int Controller::attachImage(int device, const char* image, const char** curr_values, int curr_val_size)
{
	image = extractFile(image);

	if (!image)
		return -1;

	// Check if device is correct.
	int type = getImageType(image);
	switch (type){
	case IMAGE_DISK:
	case IMAGE_PROGRAM:
		if (device != DRIVE8) return -1;
		break;
	case IMAGE_TAPE:
		if (device != DATASETTE) return -1;
		break;
	case IMAGE_CARTRIDGE:
		if (device != CARTRIDGE) return -1;
		break;
	}
	
	// Detach old image and deallocate list values
	if (curr_values && curr_val_size > 0)
		detachImage(device, curr_values, curr_val_size);
	
	image_contents_t* content = NULL;
	

	switch (device){
	case DRIVE8:
		if (attachDriveImage(image) < 0) 
			return -1;
		content = diskcontents_read(image, 8);
		break;
	case DATASETTE:
		if (attachTapeImage(image) < 0) 
			return -1;
		content = tapecontents_read(image);
		break;
	case CARTRIDGE:
		if (attachCartridgeImage(image) < 0)
			return -1;
		break;
	}

	const char** list_values = 0;
	int list_size = 0;

	if(content){
		// Retrieve content list.
		image_contents_file_list_t* entry = content->file_list;
	
		// Get list size;
		while (entry){
			list_size++;
			entry = entry->next;
		}

		if (list_size){
			list_values = new const char*[list_size];
			const char** p = list_values;
			entry = content->file_list;
			for (int i=0; i<list_size; ++i){
				*p = image_contents_file_to_string(entry, 1); // allocates memory from heap. 
				entry = entry->next;
				p++;
			}
		}
	}

	string value = list_size? list_values[0]: getFileNameFromPath(image).c_str();
	gs_view->onSettingChanged(device, value.c_str(), image, list_values, list_size, 15);

	if (content)
		image_contents_destroy(content);

	return 0;
}

void Controller::detachImage(int peripheral, const char** values, int size)
{
	switch (peripheral){
	case DRIVE8:
		detachDriveImage();
		break;
	case DATASETTE:
		detachTapeImage();
		break;
	case CARTRIDGE:
		detachCartridgeImage();
	}

	// Deallocate listbox values if any.
	if (values && size){
		const char** p = values;
		for (int i=0; i<size; ++i){
			lib_free(*p);
			p++;
		}
		delete[] values;
	}

	gs_view->onSettingChanged(peripheral,"Empty","",0,0,15);
}

const char* Controller::extractFile(const char *path)
{
  const char* game_path = path;
  char* file_buffer = NULL;
  int file_size = 0;

  if (isZipFile(path)){

    char archived_file[512];
	int image_type;
    unzFile zipfile = NULL;
    unz_global_info gi;
    unz_file_info fi;

    // Open Zip for reading
    if (!(zipfile = unzOpen(path))){
		return NULL;
	}

    // Get Zip file information
    if (unzGetGlobalInfo(zipfile, &gi) != UNZ_OK){
		unzClose(zipfile);
		return NULL;
    }

    const char *extension;
    int i, j;

	// Get the first file in the archive.
	for (i = 0; i < (int)gi.number_entry; i++){
		// Get name of the archived file
		if (unzGetCurrentFileInfo(zipfile, &fi, archived_file, sizeof(archived_file), NULL, 0, NULL, 0) != UNZ_OK){
			unzClose(zipfile);
			return NULL;
		}

		image_type = getImageType(archived_file);
		file_size = fi.uncompressed_size;
		
		if (file_size > 0 && 
			image_type == IMAGE_DISK || 
			image_type == IMAGE_TAPE || 
			image_type == IMAGE_CARTRIDGE || 
			image_type == IMAGE_PROGRAM){

			// Supported file found. 
				
			// Open file for reading
			if(unzOpenCurrentFile(zipfile) != UNZ_OK){
				unzClose(zipfile); 
				return NULL;
			}

			file_buffer = new char[file_size];
				
			unzReadCurrentFile(zipfile, file_buffer, file_size);
			unzCloseCurrentFile(zipfile);
			unzClose(zipfile);

			FileExplorer fileExp;

			if (m_zipFileSlots.size() < image_type + 1){
				delete[] file_buffer;
				return NULL;
			}

			// Delete previous temp image file.
			if (!m_zipFileSlots[image_type].empty()){
				fileExp.deleteFile(m_zipFileSlots[image_type].c_str());
			}

			// Define new temp image file name.
			m_zipFileSlots[image_type] = TMP_DIR;
			m_zipFileSlots[image_type].append(archived_file);
	
			// Create image file from the buffer data.
			FILE *file = fopen(m_zipFileSlots[image_type].c_str(), "w");
			if (!file){
				m_zipFileSlots[image_type].clear();
				return NULL;
			}
			if (fwrite(file_buffer, 1, file_size, file) < file_size){
				fclose(file);
				m_zipFileSlots[image_type].clear();
				return NULL;
			}
			fclose(file);

			game_path = m_zipFileSlots[image_type].c_str();

			delete[] file_buffer;

			return game_path;
        }
	
		// Go to the next file in the archive
		if (i + 1 < (int)gi.number_entry){
			if (unzGoToNextFile(zipfile) != UNZ_OK){
				unzClose(zipfile);
				return NULL;
			}
		}
    }

    // No supported files found
    return NULL;
  }

  return game_path;
}

static void toggleJoystickPorts()
{
	if (g_joystickPort == 1){
		g_joystickPort = 2;
		resources_set_int(VICE_RES_JOY_PORT1_DEV, 0); // 0 = None  
		resources_set_int(VICE_RES_JOY_PORT2_DEV, 1); // 1 = Joystick
		gs_view->onSettingChanged(JOYSTICK_PORT,"Port 2","",0,0,1);
	}
	else{
		g_joystickPort = 1;
		resources_set_int(VICE_RES_JOY_PORT1_DEV, 1); 
		resources_set_int(VICE_RES_JOY_PORT2_DEV, 0); 
		gs_view->onSettingChanged(JOYSTICK_PORT,"Port 1","",0,0,1);
	}
}

static void toggleWarpMode()
{
	int value;
    if (resources_get_int(VICE_RES_WARP_MODE, &value) < 0)
        return;

	value = value? 0:1;
	resources_set_int(VICE_RES_WARP_MODE, value);
}

static void	checkPendingActions()
{	
	// Check for pending actions. This function is called at the end of each screen frame. 
	// That's every 20ms in PAL standard and every 16ms in NTSC standard.
	
	if (gs_showMenuTimer > 0){
		if (--gs_showMenuTimer == 0)
			video_psv_menu_show();
	}
	if (gs_pauseTimer > 0){
		if (--gs_pauseTimer == 0){
			ui_pause_emulation(1);
			gs_view->displayPaused(1);
			gs_view->setFPSCount(0,0,0);
			gs_view->updateView();
		}
	}
	if (gs_loadDiskTimer > 0){
		if (--gs_loadDiskTimer == 0){
			if (!isCpuInRam()){ // We want to be in ROM.
				char* cmd = lib_msprintf("LOAD\"%s\",8,1:\r", gs_loadProgramName.c_str());
				kbdbuf_feed(cmd);
				lib_free(cmd);
				// Schedule screen scan command.
				setPendingAction(CTRL_ACTION_SCAN_SCREEN_READY);
			}
		}
	}
	if (gs_loadTapeTimer > 0){
		if (--gs_loadTapeTimer == 0){
			if (!isCpuInRam()){ // We want to be in ROM.
				//PSV_DEBUG("Type LOAD command...");
				if (gs_loadProgramName.empty())
					kbdbuf_feed("LOAD:\r");
				else{
					char* cmd = lib_msprintf("LOAD\"%s\":\r", gs_loadProgramName.c_str());
					kbdbuf_feed(cmd);
					lib_free(cmd);
				}

				datasette_control(DATASETTE_CONTROL_START);
				setPendingAction(CTRL_ACTION_SCAN_SCREEN_READY);
			}
		}
	}
	if (gs_kbdCmdRunTimer > 0){
		if (--gs_kbdCmdRunTimer == 0){
			kbdbuf_feed("RUN\r");
			// Update g_game_file so we can see the save states.
			g_game_file = gs_loadImageName;
			// Load game specific settings.
			gs_view->updateSettings();
		}
	}
	if (gs_scanScreenReadyTimer > 0){
		if (--gs_scanScreenReadyTimer == 0){
			int ret = scanScreen("READY.", CURSOR_WAIT_BLINK);

			if (ret == 0){ // "READY." printed on screen
				// Type "RUN" command.
				//PSV_DEBUG("READY. printed on screen. Type RUN command...");
				setPendingAction(CTRL_ACTION_KBDCMD_RUN);
			}
			else if (ret == 1){ // "READY." not printed on screen
				//PSV_DEBUG("READY. not printed on screen!");
			}
			else if (ret == 2){ // Waiting for print
				// The routine that is responsible for writing text on screen is in ROM.
				if (!isCpuInRam()){
					// Still in ROM. Keep waiting.
					//PSV_DEBUG("Waiting for READY print...");
					gs_scanScreenReadyTimer = 50;
				}
			}
		}
	}
}

static void setPendingAction(CTRL_ACTION action)
{
	// Set actions that for some reason need to be performed after a delay or during emulation.
	
	// Show menu and pause:
	// Drain the audible sound buffer before the action. 
	// This is done to prevent hearing remains of previous game audio when loading a new game.
	// The trick here is to just put the volume down, wait about 200ms until there is
	// silence in the buffer and then performing the action.
	// TODO: Find a better way to empty the sound buffer.
	//
	// Load tape/disk:
	// When loading files from peripherals we need to delay the load for the property changes
	// to be in effect. For instance, if you change drive sound to enabled just before you load,
	// you won't hear anything. Same thing when you change drive mode to true, the load will get stuck.
	//
	// Run commmand:
	// The Run command must be fed to the keyboard queue after the loading commands have been
	// printed to the screen. The command will stay in the queue until the ready message appears
	// with the blinking cursor.
	//
	// Scan screen:
	// Scan screen memory for a specific string. When attaching and loading game images from Devices-menu 
	// we get into a lot of problems identifying savestate locations. Triggering savestate location after
	// an attachement operation is problematic. If for instance you are playing Bruce Lee and then go to
	// Devices-menu and attach Giana Sister image in a Drive or Datasette, your saves will go to Giana Sisters 
	// saveslots. But more importantly if you play multidisk games your savestates will be scattered to several 
	// disk images and they become unreachable from the main disk. The right way to go is to trigger the savestate
	// location after the Autoload-operation. This way we will have a new location only when loading a new game.
	// To achieve this we will have to know what's going on the screen. If we can identify a "READY." text
	// followed by a blinking cursor we know that we have loaded a program to the computer.
	
	switch (action){

	case CTRL_ACTION_SHOW_MENU:
		if (!gs_showMenuTimer){
			gs_showMenuTimer = 10;
			setSoundVolume(0); 
		}
		break;
	case CTRL_ACTION_PAUSE:
		if (!gs_pauseTimer){
			gs_pauseTimer = 10;
			setSoundVolume(0); 
		}
		break;
	case CTRL_ACTION_LOAD_DISK:
		if (!gs_loadDiskTimer){
			gs_loadDiskTimer = 50;
		}
		break;
	case CTRL_ACTION_LOAD_TAPE:
		if (!gs_loadTapeTimer){
			gs_loadTapeTimer = 50;
		}
		break;
	case CTRL_ACTION_KBDCMD_RUN:
		if (!gs_kbdCmdRunTimer){
			gs_kbdCmdRunTimer = 5;
		}
		break;
	case CTRL_ACTION_SCAN_SCREEN_READY:
		if (!gs_scanScreenReadyTimer){
			gs_scanScreenReadyTimer = 50;
		}
		break;
	}
}

static void setSoundVolume(int vol)
{
	resources_set_int(VICE_RES_SOUND_VOLUME, vol); 
}

static void pauseEmulation(bool pause)
{
	if (pause){
		if (ui_emulation_is_paused())
			return;

		ui_pause_emulation(1);
		gs_view->displayPaused(1);		

	}
	else{
		if (!ui_emulation_is_paused())
			return;

		ui_pause_emulation(0);
		gs_view->displayPaused(0);	
	}
}

bool Controller::isZipFile(const char* fname)
{
	string extension;
	string file = fname;
	bool ret = false;

	size_t dot_pos = file.find_last_of(".");
	if (dot_pos != string::npos)
		extension = file.substr(dot_pos+1, string::npos);

	if (extension.empty())
		return false;
	
	strToUpperCase(extension);

	if (extension == "ZIP")
		ret = true;

	return ret;
}

static int scanScreen(const char *s, unsigned int blink_mode)
{
	// Scans the screen memory for the text in parameter s.
	// blink_mode parameter specifies if s is followed by a new line and a blinking/flashing cursor.
	// Return values:
	// 0 = text in s was printed on screen.
	// 1 = text in s was not printed on screen.
	// 2 = waiting for a text to be printed.
	
	// C64 specific values. Use different for C128.
	int blnsw = 0xcc;
	int pnt   = 0xd1;
	int pntr  = 0xd3;
	int lnmx  = 0xd5;
	
    int screen_addr, line_length, cursor_column, addr, i;

    screen_addr = (int)(mem_read((uint16_t)(pnt)) | (mem_read((uint16_t)(pnt + 1)) << 8));
    cursor_column = (int)mem_read((uint16_t)(pntr));

    line_length = (int)(lnmx < 0 ? -lnmx : mem_read((uint16_t)(lnmx)) + 1);

    if (!kbdbuf_is_empty()) {
        return 2;
    }

    if (blink_mode == CURSOR_WAIT_BLINK && cursor_column != 0) {
        return 2;
    }

    if (blink_mode == CURSOR_WAIT_BLINK && blnsw != 0 && mem_read(blnsw) != 0) {
        return 2;
    }

    if (blink_mode == CURSOR_WAIT_BLINK) {
        addr = screen_addr - line_length;
    } else {
        addr = screen_addr;
    }

    for (i = 0; s[i] != '\0'; i++) {
        if (mem_read((uint16_t)(addr + i)) != s[i] % 64) {
            if (mem_read((uint16_t)(addr + i)) != (uint8_t)32) {
                return 1; // NO
            }
            return 2; // NOT YET
        }
    }
    return 0; // YES
}

static bool isCpuInRam()
{
	// Checks if CPU is currently running in RAM area.
	if (machine_addr_in_ram(reg_pc)) {
		return true;
	}
  
	return false;
}