
#ifndef CTRL_DEFS_H
#define CTRL_DEFS_H

enum CTRL_ACTION {
	CTRL_ACTION_SHOW_MENU = 0, 
	CTRL_ACTION_PAUSE,
	CTRL_ACTION_LOAD_DISK,
	CTRL_ACTION_LOAD_TAPE,
	CTRL_ACTION_KBDCMD_RUN
};

static bool	  gs_frameDrawn = false;
static bool   gs_bootTime = true;	
static int	  gs_showMenuTimer = 0;
static int	  gs_pauseTimer = 0;
static int	  gs_loadDiskTimer = 0;
static int	  gs_loadTapeTimer = 0;
static int	  gs_kbdCmdRunTimer = 0;
static bool   gs_scanMouse = false;
static int	  gs_machineResetMode = 1;
static string gs_loadProgramName;
int			  g_joystickPort = 2;

static void	 toggleJoystickPorts();
static void	 toggleWarpMode();
static void	 setPendingAction(CTRL_ACTION);
static void	 checkPendingActions();
static void	 setSoundVolume(int);

#endif

