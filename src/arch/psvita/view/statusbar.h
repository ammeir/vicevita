
/* statusbar.h: Statusbar implementation.

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

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <string>

#define IMG_SB_STATUSBAR               0
#define IMG_SB_LED_ON_GREEN            1
#define IMG_SB_LED_ON_RED              2
#define IMG_SB_LED_OFF                 3
#define IMG_SB_TAPE_STOP_MOTOR_ON      4
#define IMG_SB_TAPE_START_MOTOR_ON     5 
#define IMG_SB_TAPE_START_MOTOR_OFF    6
#define IMG_SB_TAPE_FORWARD_MOTOR_ON   7
#define IMG_SB_TAPE_FORWARD_MOTOR_OFF  8
#define IMG_SB_TAPE_REWIND_MOTOR_ON    9
#define IMG_SB_TAPE_REWIND_MOTOR_OFF  10
#define IMG_SB_TAPE_RECORD_MOTOR_ON   11
#define IMG_SB_TAPE_RECORD_MOTOR_OFF  12
#define IMG_SB_NULL					  13

typedef struct{
	int		number;
	int		led;
	char	track[8];
	//float	track;
	int		bitmask;
}drive_s;


using std::string;

class View;
class vita2d_texture;
class Statusbar
{

private:
	View*			m_view;
	drive_s			m_drives[4];
	vita2d_texture* m_bitmaps[16];
	vita2d_texture* m_tapeControlTex;
	char			m_track[8];
	char			m_fps[8];
	char			m_cpu[8];
	char			m_counter[8];
	int				m_warpFlag;
	int				m_tapeControl;
	int				m_tapeMotor;
	int				m_lastActiveDrive;
	char			m_driveLedMask;
	char			m_driveDiskMask;
	bool			m_updated;
	
	void			loadResources();

public:
					Statusbar();
					~Statusbar();

	void			init(View*);
	void			show();
	int				render();
	void			setSpeedData(int fps, int percent, int warp_flag);
	void			setTapeCounter(int counter);
	void			setTapeControl(int control);
	void			setDriveLed(int drive, int led);
	void			setTapeMotor(int motor);
	void			setDriveTrack(unsigned int drive, unsigned int half_track);
	void			setDriveDiskPresence(int drive, int disk_in);
	bool			isUpdated();
	void			notifyReset();
};

#endif