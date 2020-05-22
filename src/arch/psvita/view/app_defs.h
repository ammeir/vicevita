
#ifndef APP_DEFS_H
#define APP_DEFS_H

#define APP_NAME				"VICEVita"
#define	APP_DATA_DIR			"ux0:data/vicevita/" 
#define GAME_DIR APP_DATA_DIR	"games/"
#define SAVE_DIR APP_DATA_DIR	"saves/"
#define VICE_DIR APP_DATA_DIR	"vice/"
#define TMP_DIR APP_DATA_DIR	"tmp/"

// ux0:app is not accessible by regular apps but they can access their own resources through app0:
// which is mounted to point to their own directory and is also mounted as read only.
#define APP_RESOURCES			"app0:/resources"

#define CONF_FILE_NAME			"config.ini"

// Default configuration file
#define DEF_CONF_FILE_PATH APP_DATA_DIR CONF_FILE_NAME

// VICE resource strings
#define VICE_RES_CARTRIDGE_RESET			"CartridgeReset"
#define VICE_RES_DATASETTE_RESET_WITH_CPU	"DatasetteResetWithCPU"
#define VICE_RES_DRIVE_TRUE_EMULATION		"DriveTrueEmulation"
#define VICE_RES_DRIVE_SOUND_EMULATION		"DriveSoundEmulation"
#define VICE_RES_JOY_DEVICE_1				"JoyDevice1"
#define VICE_RES_JOY_DEVICE_2				"JoyDevice2"
#define VICE_RES_JOY_PORT1_DEV				"JoyPort1Device"
#define VICE_RES_JOY_PORT2_DEV				"JoyPort2Device"
#define VICE_RES_MACHINE_VIDEO_STANDARD		"MachineVideoStandard"
#define VICE_RES_MOUSE						"Mouse"
#define VICE_RES_SID_ENGINE					"SidEngine"
#define VICE_RES_SID_MODEL					"SidModel"
#define VICE_RES_SID_RESID_SAMPLING			"SidResidSampling"
#define VICE_RES_SOUND						"Sound"
#define VICE_RES_SOUND_VOLUME				"SoundVolume"
#define VICE_RES_CPU_SPEED					"Speed"
#define VICE_RES_VICII_FILTER				"VICIIfilter"
#define VICE_RES_VICII_DOUBLE_SCAN			"VICIIDoubleScan"
#define VICE_RES_VICII_DOUBLE_SIZE			"VICIIDoubleSize"
#define VICE_RES_VICII_EXTERNAL_PALETTE		"VICIIExternalPalette"
#define VICE_RES_VIRTUAL_DEVICES			"VirtualDevices"
#define VICE_RES_WARP_MODE					"WarpMode"

// Settings/Peripherals entry id's
#define KEYMAPS						1
#define VICII_MODEL					2
#define SID_ENGINE					3
#define SID_MODEL					4
#define ASPECT_RATIO				5
#define TEXTURE_FILTER				6
#define COLOR_PALETTE				7
#define BORDERS						8
#define JOYSTICK_PORT				9
#define JOYSTICK_SIDE				10
#define JOYSTICK_AUTOFIRE_SPEED		11
#define KEYBOARD_MODE				12
#define CPU_SPEED					13
#define HOST_CPU_SPEED				14
#define FPS_COUNTER					15
#define SOUND						16		
#define DRIVE8						17
#define DRIVE_TRUE_EMULATION		18
#define DRIVE_SOUND_EMULATION		19
#define DATASETTE					20
#define DATASETTE_CONTROL			21
#define DATASETTE_COUNTER			22
#define DATASETTE_RESET_WITH_CPU	23
#define CARTRIDGE					24
#define CARTRIDGE_RESET				25
#define MACHINE_RESET				26
#define MOUSE						27
#define MOUSE_MODEL					28
#define MOUSE_PORT					29
#define SETTINGS_ALL				30
#define SETTINGS_VIEW				31
#define SETTINGS_MODEL				32
#define SETTINGS_MODEL_NOT_IN_SNAP	33

// Setting types
#define ST_MODEL					1 
#define ST_VIEW						2 

// Image file type
#define IMAGE_DISK					0
#define IMAGE_TAPE					1
#define IMAGE_CARTRIDGE				2
#define IMAGE_PROGRAM				3

// Snapshot patch modules. Max 16 letters.
#define SNAP_MOD_THUMB				"SMOD_THUMB"
#define SNAP_MOD_SETTINGS			"SMOD_SETTINGS"
// Patch module delimeters
#define SNAP_MOD_DELIM_ENTRY		"|"
#define SNAP_MOD_DELIM_FIELD		"^"

// Cartridge control id's
#define CART_CONTROL_FREEZE			1
#define CART_CONTROL_SET_DEFAULT	2
#define CART_CONTROL_FLUSH_IMAGE	3
#define CART_CONTROL_SAVE_IMAGE		4

// Tape control id's
#define TAPE_CONTROL_STOP			0
#define TAPE_CONTROL_PLAY			1
#define TAPE_CONTROL_FORWARD		2
#define TAPE_CONTROL_REWIND			3
#define TAPE_CONTROL_RECORD			4
#define TAPE_CONTROL_RESET			5
#define TAPE_CONTROL_RESET_COUNTER	6

// Button image id's
#define IMG_BTN_NAVIGATE_UP_DOWN		0
#define IMG_BTN_NAVIGATE_UP_DOWN_LEFT	1
#define IMG_BTN_NAVIGATE_UP_DOWN_X		2
#define IMG_BTN_DPAD_LEFT_BLUE			3 
#define IMG_BTN_TRIANGLE_RED			4
#define IMG_BTN_TRIANGLE_MAGENTA		5
#define IMG_BTN_CIRCLE_GREEN			6
#define IMG_BTN_CIRCLE_YELLOW			7
#define IMG_BTN_CROSS_GREEN				8
#define IMG_BTN_SQUARE_MAGENTA			9
#define IMG_BTN_LTRIGGER_BLUE			10
#define IMG_BTN_RTRIGGER_BLUE			11
#define IMG_BTN_CIRCLE_BLUE				12
#define IMG_BTN_CROSS_BLUE				13
#define IMG_BTN_SQUARE_BLUE				14
#define IMG_BTN_TRIANGLE_BLUE			15

#define BLACK				RGBA8(0, 0, 0, 255)
#define GREY				RGBA8(128, 128, 128, 255)
#define LIGHT_GREY			RGBA8(182, 182, 182, 255)
#define DARK_GREY			RGBA8(64, 64, 64, 255)
#define DARK_GREY2			RGBA8(45, 45, 45, 255)
#define WHITE				RGBA8(255, 255, 255, 255)
#define YELLOW_TRANSPARENT  RGBA8(255, 255, 0, 60)
#define DARK_YELLOW			RGBA8(204,204,0,255)
#define LIGHT_YELLOW		RGBA8(255,255,51,255)
#define YELLOW				RGBA8(192,192,0,255)
#define BLUE				RGBA8(0, 0, 200, 255)
#define ROYAL_BLUE			RGBA8(65, 105, 255, 255)
#define CYAN				RGBA8(0, 192, 192, 255)
#define C64_BLUE			RGBA8(79, 67, 215, 255)


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b)) 

#endif
