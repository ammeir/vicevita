
#ifndef CTRL_DEFS_H
#define CTRL_DEFS_H

enum CTRL_ACTION {
	CTRL_ACTION_SHOW_MENU = 0, 
	CTRL_ACTION_PAUSE,
	CTRL_ACTION_LOAD_DISK,
	CTRL_ACTION_LOAD_TAPE,
	CTRL_ACTION_KBDCMD_RUN,
	CTRL_ACTION_SCAN_SCREEN_READY

};

#define CURSOR_WAIT_BLINK   0
#define CURSOR_NOWAIT_BLINK 1


static bool	  gs_frameDrawn = false;
static bool   gs_bootTime = true;	
static bool	  gs_autofireOn = false;
static int	  gs_autofireSpeed = 0;
static int    gs_moduloDivider = 3;
static int    gs_frameCounter = 0;
static int	  gs_showMenuTimer = 0;
static int	  gs_pauseTimer = 0;
static int	  gs_loadDiskTimer = 0;
static int	  gs_loadTapeTimer = 0;
static int	  gs_kbdCmdRunTimer = 0;
static int	  gs_scanScreenReadyTimer = 0;
static bool   gs_scanMouse = false;
static int	  gs_machineResetMode = 1;
static string gs_loadProgramName;
static string gs_loadImageName;
int			  g_joystickPort = 2;

static void	 toggleJoystickPorts();
static void	 toggleWarpMode();
static void	 setPendingAction(CTRL_ACTION);
static void	 checkPendingActions();
static void	 setSoundVolume(int);
static void	 pauseEmulation(bool pause);
static int	 scanScreen(const char *s, unsigned int blink_mode);
static bool	 isCpuInRam();


#endif

