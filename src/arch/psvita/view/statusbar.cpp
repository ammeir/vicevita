
/* statusbar.cpp: Statusbar implementation.

   Copyright (C) 2019-2021 Amnon-Dan Meir.

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

#include "statusbar.h"
#include "view.h"
#include "texter.h"
#include "resources.h"
#include "app_defs.h"
#include <string.h> // memcpy
#include <vita2d.h>


Statusbar::Statusbar()
{
	m_updated = false;
	m_driveLedMask = 0;
	m_tapeMotor = 0;
	m_tapeControl = 0;
	m_tapeControlTex = NULL;
}

Statusbar::~Statusbar()
{
	int i = 0;
	while (m_bitmaps[i]){
		vita2d_free_texture (m_bitmaps[i++]);
	}

}

void Statusbar::init(View* view)
{
	m_view = view;

	loadResources();

	sprintf(m_counter, "000");
	sprintf(m_cpu, "000%%");
	sprintf(m_fps, "00");
}

void Statusbar::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing();
    vita2d_swap_buffers();
}

int Statusbar::render()
{
	// Statusbar layout. Drawing layout is faster than drawing it from scratch.
	vita2d_draw_texture(m_bitmaps[IMG_SB_STATUSBAR], 0, 513);

	// Drive leds.
	if (m_driveLedMask){
		if (m_driveLedMask & 0x01)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 77, 522); 
		if (m_driveLedMask & 0x02)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 102, 522);
		if (m_driveLedMask & 0x04)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 127, 522); 
		if (m_driveLedMask & 0x08)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 152, 522); 
	}

	// Tape control texture.
	if (m_tapeControlTex)
		vita2d_draw_texture(m_tapeControlTex, 259, 520); 

	// Tape counter, fps, cpu percentage.
	txtr_draw_text(385, 534, YELLOW, m_counter);
	txtr_draw_text(498, 534, YELLOW, m_fps);
	txtr_draw_text(598, 534, YELLOW, m_cpu);
	
	// Warp led.
	if (m_warpFlag)
		vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_GREEN], 744, 522);

	m_updated = false;
	return 1;
}

void Statusbar::setSpeedData(int fps, int cpu, int warp_flag)
{
	static int prev_fps = 0;
	static int prev_cpu = 0;
	
	m_warpFlag = warp_flag;

	if (cpu != prev_cpu)
		cpu < 100? sprintf(m_cpu, "0%d%%", cpu): sprintf(m_cpu, "%d%%", cpu);
	if (fps != prev_fps)
		fps < 10? sprintf(m_fps, "0%d", fps): sprintf(m_fps, "%d", fps);
	
	prev_fps = fps;
	prev_cpu = cpu;

	m_updated = true;
}

void Statusbar::setDriveLed(int drive, int led)
{
	switch(drive){
	case 8:
		m_driveLedMask = led? m_driveLedMask | 0x01: m_driveLedMask & 0xFE;
		break;
	case 9:
		m_driveLedMask = led? m_driveLedMask | 0x02: m_driveLedMask & 0xFD;
		break;
	case 10:
		m_driveLedMask = led? m_driveLedMask | 0x04: m_driveLedMask & 0xFB;
		break;
	case 11:
		m_driveLedMask = led? m_driveLedMask | 0x08: m_driveLedMask & 0xF7;
		break;
	}
	
	m_updated = true;
}

void Statusbar::setTapeControl(int control)
{
	string str;

	switch (control){
	case 0:
		str = "Stop"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_STOP_MOTOR_ON]: NULL;
		break;
	case 1: 
		str = "Play"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_START_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_START_MOTOR_OFF];
		break;
	case 2: 
		str = "Forward"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_OFF];
		break;
	case 3: 
		str = "Rewind"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_OFF];
		break;
	case 4: 
		str = "Record";
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_OFF];
		break;
	case 5: 
		str = "Reset";
		break;
	default: 
		str = "Stop";
		m_tapeControlTex =  m_tapeMotor? m_bitmaps[IMG_SB_TAPE_STOP_MOTOR_ON]: NULL;
	}

	// Send update to peripherals too. 
	m_view->onSettingChanged(DATASETTE_CONTROL, str.c_str(),0,0,0,1);
	m_tapeControl = control;
	m_updated = true;
}

void Statusbar::setTapeCounter(int counter)
{
	switch (counter){
	case 0 ... 9:
		sprintf(m_counter, "00%d", counter);
		break;
	case 10 ... 99:
		sprintf(m_counter, "0%d", counter);
		break;
	case 100 ... 999:
		sprintf(m_counter, "%d", counter);
		break;
	default:
		sprintf(m_counter, "000", counter);
		break;
	}
	
	m_updated = true;
}

void Statusbar::setTapeMotor(int motor)
{
	m_tapeMotor = motor;
	setTapeControl(m_tapeControl);
}

bool Statusbar::isUpdated()
{
	return m_updated;
}

void Statusbar::loadResources()
{
	m_bitmaps[IMG_SB_STATUSBAR] = vita2d_load_PNG_buffer(img_statusbar);
	m_bitmaps[IMG_SB_LED_ON_GREEN] = vita2d_load_PNG_buffer(img_led_on_green);
	m_bitmaps[IMG_SB_LED_ON_RED] = vita2d_load_PNG_buffer(img_led_on_red);
	m_bitmaps[IMG_SB_LED_OFF] = vita2d_load_PNG_buffer(img_led_off);
	m_bitmaps[IMG_SB_TAPE_STOP_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_stop_motor_on);
	m_bitmaps[IMG_SB_TAPE_START_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_start_motor_on);
	m_bitmaps[IMG_SB_TAPE_START_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_start_motor_off);
	m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_forward_motor_on);
	m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_forward_motor_off);
	m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_rewind_motor_on);
	m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_rewind_motor_off);
	m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_record_motor_on);
	m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_record_motor_off);
	m_bitmaps[IMG_SB_NULL] = NULL;
}
