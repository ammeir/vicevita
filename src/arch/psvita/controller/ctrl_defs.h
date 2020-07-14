
#ifndef CTRL_DEFS_H
#define CTRL_DEFS_H

enum ctrl_pending_action_e{
	CTRL_ACTION_SHOW_MENU = 0, 
	CTRL_ACTION_PAUSE,
	CTRL_ACTION_ACTIVATE_DRIVE,
	CTRL_ACTION_DEACTIVATE_DRIVE,
	CTRL_ACTION_KBDCMD_LOAD_DISK,
	CTRL_ACTION_KBDCMD_LOAD_TAPE,
	CTRL_ACTION_KBDCMD_RUN,
	CTRL_ACTION_SCANSCR_PRESSPLAYONTAPE,
	CTRL_ACTION_SCANSCR_LOADING,
	CTRL_ACTION_SCANSCR_LOADING_READY

};

#define CURSOR_WAIT_BLINK   0
#define CURSOR_NOWAIT_BLINK 1


static bool	  gs_frameDrawn = false;
static bool   gs_bootTime = true;	
static bool	  gs_autofireOn = false;
static bool   gs_autoStartInProgress = false;
static int	  gs_autofireSpeed = 0;
static int    gs_moduloDivider = 3;
static int    gs_frameCounter = 0;
static int	  gs_showMenuTimer = 0;
static int	  gs_pauseTimer = 0;
static int	  gs_loadDiskTimer = 0;
static int	  gs_loadTapeTimer = 0;
static int	  gs_kbdCmdRunTimer = 0;
static int	  gs_activateDriveTimer = 0;
static int	  gs_activateDriveAndLoadDiskTimer = 0;
static int	  gs_deactivateDriveTimer = 0;
static int    gs_scanScreenPressPlayTimer = 0;
static int    gs_scanScreenLoadingTimer = 0;
static int	  gs_scanScreenReadyTimer = 0;
static bool   gs_scanMouse = false;
static int	  gs_machineResetMode = 1;
static string gs_loadProgramName;
int			  g_joystickPort = 2;

static void	 toggleJoystickPorts();
static void	 toggleWarpMode();
static void	 setPendingAction(ctrl_pending_action_e);
static void	 checkPendingActions();
static void	 setSoundVolume(int);
static void	 pauseEmulation(bool pause);
static int	 scanScreen(const char *s, unsigned int blink_mode);
static bool	 isCpuInRam();
static bool	 isFileOfType(const char* fname, const char* type);
static void	 strToUpperCase(string& str);
static int   getCurrentDriveId();
static bool  isTapOnTape();


#endif

