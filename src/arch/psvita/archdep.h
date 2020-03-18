/*
 * archdep.h - Miscellaneous system-specific stuff.
 *
 * Written by
 *  Amnon-Dan Meir <ammeir71@yahoo.com>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_ARCHDEP_H
#define VICE_ARCHDEP_H

#include "sound.h" 

/* Filesystem dependant operators.  */
#define FSDEVICE_DEFAULT_DIR   "."
#define FSDEV_DIR_SEP_STR      "/"
#define FSDEV_DIR_SEP_CHR      '/'
#define FSDEV_EXT_SEP_STR      "."
#define FSDEV_EXT_SEP_CHR      '.'

/* Path separator.  */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR         ';'
#define ARCHDEP_FINDPATH_SEPARATOR_STRING       ";"

#define ARCHDEP_DIR_SEPARATOR					'/'

/* Modes for fopen().  */
#define MODE_READ              "r"
#define MODE_READ_TEXT         "r"
#define MODE_READ_WRITE        "r+"
#define MODE_WRITE             "w"
#define MODE_WRITE_TEXT        "w"
#define MODE_APPEND            "w+"
#define MODE_APPEND_READ_WRITE "a+"

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "print.dump"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "|lpr"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "|petlp -F PS|lpr"

/* Video chip scaling.  */
#define ARCHDEP_VICII_DSIZE   0
#define ARCHDEP_VICII_DSCAN   0
#define ARCHDEP_VICII_HWSCALE 1

/* Video chip double buffering.  */
#define ARCHDEP_VICII_DBUF 0

/* Default RS232 devices.  */
#define ARCHDEP_RS232_DEV1 "/dev/ttyS0"
#define ARCHDEP_RS232_DEV2 "/dev/ttyS1"
#define ARCHDEP_RS232_DEV3 "rs232.dump"
#define ARCHDEP_RS232_DEV4 "|lpr"

/* Default location of raw disk images.  */
#define ARCHDEP_RAWDRIVE_DEFAULT "/dev/fd0"

/* Access types */
#define ARCHDEP_R_OK R_OK
// Checking write permission (W_OK) with access() fails for some reason and makes prg/p00 files unplayable.
// This needs further studying. In the mean time just check file existance F_OK instead. 
#define ARCHDEP_W_OK F_OK 
#define ARCHDEP_X_OK X_OK
#define ARCHDEP_F_OK F_OK

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\n"

/* Ethernet default device */
#define ARCHDEP_ETHERNET_DEFAULT_DEVICE "eth0"

/* Default sound fragment size */
#define ARCHDEP_SOUND_FRAGMENT_SIZE 1

/* No key symcode.  */
#define ARCHDEP_KEYBOARD_SYM_NONE 0

/* what to use to return an error when a socket error happens */
#define ARCHDEP_SOCKET_ERROR errno

/* Default sound output mode */
#define ARCHDEP_SOUND_OUTPUT_MODE SOUND_OUTPUT_SYSTEM

/* Keyword to use for a static prototype */
#define STATIC_PROTOTYPE static 

/** \brief  Autostart diskimage prefix */
#define ARCHDEP_AUTOSTART_DISKIMAGE_PREFIX  "autostart-"

/** \brief  Autostart diskimage suffix */
#define ARCHDEP_AUTOSTART_DISKIMAGE_SUFFIX  ".d64"

#define ARCHDEP_VICERC_NAME   "vicerc"

/* set this path to customize the preference storage */ 
extern const char*	archdep_pref_path;

extern const char*	archdep_home_path(void);
extern int			archdep_vice_atexit(void (*function)(void));
extern void			archdep_vice_exit(int excode);
extern int			archdep_register_cbmfont(void);
extern void			archdep_unregister_cbmfont(void);
extern char*		archdep_join_paths(const char *path, ...);
extern FILE*		archdep_open_default_log_file(void);
extern const char*	archdep_program_name(void);
extern void			archdep_program_name_free(void);
extern char*		archdep_vice_resource_path(void);
extern void			archdep_vice_resource_path_free(void);
extern void			archdep_startup_log_error(const char *format, ...);
extern int			archdep_path_is_relative(const char *path);
extern int			archdep_expand_path(char **return_path, const char *filename);
extern char*		archdep_make_backup_filename(const char *fname);
extern char*		archdep_tmpnam(void);
extern int			archdep_spawn(const char *name, char **argv, char **pstdout_redir, const char *stderr_redir);
extern char*		archdep_filename_parameter(const char *name);
extern char*		archdep_quote_parameter(const char *name);
extern FILE*		archdep_mkstemp_fd(char **filename, const char *mode);
extern int			archdep_init(int *argc, char **argv);
extern int			archdep_mkdir(const char *pathname, int mode);
extern int			archdep_rmdir(const char *pathname);
extern int			archdep_stat(const char *file_name, unsigned int *len, unsigned int *isdir);
extern int			archdep_rename(const char *oldpath, const char *newpath);
extern char*		archdep_default_sysfile_pathlist(const char *emu_id);
extern void			archdep_default_sysfile_pathlist_free(void);
extern char*		archdep_extra_title_text(void);
extern void			archdep_extra_title_text_free(void);
extern void			archdep_signals_init(int do_core_dumps);
extern void			archdep_signals_pipe_set(void);
extern void			archdep_signals_pipe_unset(void);

/* Resource handling. */
extern char*		archdep_default_resource_file_name(void);
/* Fliplist. */
extern char*		archdep_default_fliplist_file_name(void);
/* RTC. */
extern char*		archdep_default_rtc_file_name(void);
/* Autostart-PRG */
extern char*		archdep_default_autostart_disk_image_file_name(void);
extern int			archdep_default_logger(const char *level_string, const char *txt);
/* Free everything on exit.  */
extern void			archdep_shutdown(void);

/* Missing functions in Vita SDK */
extern char*		getwd(char *buffer);
extern int			chdir(const char *path);
extern int			mkdir(const char* path, mode_t mode);
extern int			rmdir(const char *path);
extern int			usleep(useconds_t usec);

#endif