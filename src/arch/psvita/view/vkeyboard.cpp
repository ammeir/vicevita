
/* vkeyboard.cpp: Implementation of C64 virtual keyboard.

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

#include "vkeyboard.h"
#include "view.h"
#include "controls.h"
#include "texter.h"
#include "ini_parser.h"
#include "debug_psv.h"
#include "resources.h"
#include "app_defs.h"
#include "debug_psv.h"
#include <string.h>
#include <vector>
#include <algorithm>
#include <vita2d.h>

using std::vector;
using std::list;

// Globals
int  g_keyboardStatus = KEYBOARD_DOWN;

int animationArr[27] = {
	0,10,20,30,40,50,60,70,80,
	90,100,110,120,130,140,150,160,
	170,180,190,200,210,220,230,240,
	250,264
};

VirtualKeyboard::VirtualKeyboard()
{
	m_keyMapLookup = NULL;
	m_keyboardStd = NULL;
	m_keyboardShift = NULL;
	m_keyboardCmb = NULL;
	m_keyboardCtrl = NULL;
	m_shiftLock = false;
	m_updated = false;
	m_animation = true;
	m_posX = 0;
	m_posY = 0;
	m_scaleX = 1;
	m_scaleY = 1;
}

VirtualKeyboard::~VirtualKeyboard()
{
	if (m_keyMapLookup)
		delete[] m_keyMapLookup;
	if (m_keyboardStd)
		vita2d_free_texture(m_keyboardStd);
	if (m_keyboardShift)
		vita2d_free_texture(m_keyboardShift);
	if (m_keyboardCmb)
		vita2d_free_texture(m_keyboardCmb);
	if (m_keyboardCtrl)
		vita2d_free_texture(m_keyboardCtrl);
}

void VirtualKeyboard::init(View* view, Controls* controls)
{
	m_view = view;
	m_keyMapLookup = new ControlPadMap[125];

	memset(m_keyMapLookup, 0, sizeof(ControlPadMap)*125);

	// Initialize all key maps.
	for (int i=0; i<125; ++i){
		if (controls->midToName(i)){
			m_keyMapLookup[i].mid = i;
			m_keyMapLookup[i].iskey = 1;
		}
		else if (i == 24){ 
			// Shift lock. Treat is as left shift.
			m_keyMapLookup[i].mid = 23;
			m_keyMapLookup[i].iskey = 1;
		}
	}

	m_keyboardStd = vita2d_load_PNG_buffer(img_keyboard_std);
	m_keyboardShift = vita2d_load_PNG_buffer(img_keyboard_shift);
	m_keyboardCmb = vita2d_load_PNG_buffer(img_keyboard_cmb);
	m_keyboardCtrl = vita2d_load_PNG_buffer(img_keyboard_ctrl);
	m_keyboard = m_keyboardStd;

	initAnimation();
}

void VirtualKeyboard::input(TouchCoordinates* touches, int count)
{
	if (count == 0 && m_touchBuffer.empty())
		return;
	
	// Find key downs
	vector<int> key_downs;
	for (int i=0; i<count; ++i){
		int mid = touchCoordinatesToMid(touches->x, touches->y);
		touches++;

		if (mid == -1)
			continue;

		// Add to buffer if it does not exist already
		list<int>::iterator it = find(m_touchBuffer.begin(), m_touchBuffer.end(), mid);
		if(it == m_touchBuffer.end()){
			m_touchBuffer.push_back(mid);
			m_keyMapLookup[mid].ispress = 1;
			m_updated = true;
			
			if (mid == 23 || mid == 24 || mid == 100 || mid == 114 || mid == 117){
				// Change keyboard layout
				changeLayout(mid);
				m_view->updateView();
			}
		}

		key_downs.push_back(mid);
	}

	// Find key ups by comparing new and old input
	if (key_downs.size() < m_touchBuffer.size()){

		// Make a copy of the list so we can modify the m_touchBuffer during loop
		list<int> tmp_list = m_touchBuffer;
				
		for (list<int>::iterator list_it=tmp_list.begin(); list_it!=tmp_list.end(); ++list_it){
		
			if(find(key_downs.begin(), key_downs.end(), *list_it) == key_downs.end()){
				// Old key not found in new input. Report key relese.
				if (m_keyMapLookup[*list_it].ispress){
					m_keyMapLookup[*list_it].ispress = 0; // release
					
					if (*list_it == 23 || *list_it == 100 || *list_it == 114 || *list_it == 117){ 
						// Change keyboard layout
						changeLayout(*list_it);
						m_view->updateView();
					}

					if ((*list_it == 23 || *list_it == 24) && m_shiftLock)
						m_touchBuffer.remove(*list_it); // Don't report shift release when locked.
				}
				else{
					m_touchBuffer.remove(*list_it); // release already reported, remove
					m_updated = true;
				}
			}
		}
	}
}
	
void VirtualKeyboard::getKeyMaps(ControlPadMap** maps, int* size)
{
	if (m_touchBuffer.empty())
		return;

	for (list<int>::iterator it=m_touchBuffer.begin(); it != m_touchBuffer.end(); ++it){
		ControlPadMap* map = &m_keyMapLookup[*it];
		*maps++ = map; (*size)++;
	}
}

void VirtualKeyboard::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing();
    vita2d_swap_buffers();
}

void VirtualKeyboard::render()
{
	static int i = 0;
	static int j = 0;

	if (g_keyboardStatus == KEYBOARD_UP){
		// Keyboard stationary

		vita2d_draw_texture_scale(m_keyboard, m_posX, m_posY, m_scaleX, m_scaleY);
		m_updated = false;

		if (m_touchBuffer.empty())
			return;

		int mid = m_touchBuffer.back();

		// Darken space bar if pressed.
		std::list<int>::iterator it = std::find(m_touchBuffer.begin(), m_touchBuffer.end(), 116);
		if(it != m_touchBuffer.end()){
			if (g_keyboardMode == KEYBOARD_SPLIT_SCREEN)
				vita2d_draw_rectangle(196, 479, 406, 42, GREY);
			else
				vita2d_draw_rectangle(165, 372, 450, 60, GREY);
			return;
		}

		// Ignore shifting keys.
		if (mid == 23 || mid == 24 || mid == 100 || mid == 114 || mid == 117)
			return;

		showMagnifiedKey(mid);
		return;
	}

	if (g_keyboardStatus == KEYBOARD_MOVING_UP){
		
		vita2d_draw_texture_part(
			m_keyboard, 
			46, // Pos x on vita screen
			544 - animationArr[i],// Pos y on vita screen
			0, // Pos x on keyboard bitmap
			0, // Pos y on keyboard bitmap
			868, // width of keyboard bitmap
			animationArr[i]); // height of keyboard bitmap
		
		if (i++ == 26){
			// Add a little delay by drawing the last frame few times.
			if (j++ > 7){
				g_keyboardStatus = KEYBOARD_UP;
				i = j = 0;
			}else
				i = 26; 
		}
		
		m_updated = true; // Force redraw.
		return;
	}
		
	if (g_keyboardStatus == KEYBOARD_MOVING_DOWN){

		// Add a little delay by drawing the first frame few times.
		if (j++ < 10)
			i = 0; 
		
		vita2d_draw_texture_part(
				m_keyboard, 
				46, // Pos x on vita screen
				278 + animationArr[i],// Pos y on vita screen
				0, // Pos x on keyboard bitmap
				0, // Pos y on keyboard bitmap
				868, // width of keyboard bitmap
				264 - animationArr[i]); // height of keyboard bitmap

		if (i++ == 26){
			g_keyboardStatus = KEYBOARD_DOWN;
			i = j = 0;
		}
		
		m_updated = true; // Force redraw.
		return;
	}
}

void VirtualKeyboard::setPosition(int x, int y, float scaleX, float scaleY)
{
	m_posX = x;
	m_posY = y;
	m_scaleX = scaleX;
	m_scaleY = scaleY;
}

void VirtualKeyboard::clear()
{
	// Clear any leftover keys. It's better to call this function whenever you show/hide the keyboard.
	m_touchBuffer.clear();
	m_updated = false;
}

void VirtualKeyboard::changeLayout(int mid)
{
	// Change keyboard layout when one of the shifting keys is pressed or released

	if (m_keyMapLookup[mid].ispress){
		// Key press
		switch (mid){
		case 23: // Left shift
		case 100: // Right shift
			m_keyboard = m_keyboardShift;
			break;
		case 24: // Shift lock. Toggle shift on/off.
			m_keyboard = m_shiftLock? m_keyboardStd: m_keyboardShift;
			m_shiftLock = !m_shiftLock;
			break;
		case 114: // Ctrl
			m_keyboard = m_keyboardCtrl;
			break;
		case 117: // C=
			m_keyboard = m_keyboardCmb;
			break;
		}
	}
	else{
		// Key release. Check if other shift keys are pressed.
		if (m_keyMapLookup[100].ispress || m_keyMapLookup[23].ispress)
			m_keyboard = m_keyboardShift;
		else if (m_keyMapLookup[114].ispress)
			m_keyboard = m_keyboardCtrl;
		else if  (m_keyMapLookup[117].ispress)
			m_keyboard = m_keyboardCmb;
		else if (m_shiftLock)
			m_keyboard = m_keyboardShift;
		else
			m_keyboard = m_keyboardStd;
	}
}

void VirtualKeyboard::showMagnifiedKey(int mid)
{
	static RectCoordinates rc;
	static int key_pos_x = 0;
	static int key_pos_y = 0;

	midToKeyboardCoordinates(mid, &rc);

	// Key box position on the vita screen.
	if (g_keyboardMode == KEYBOARD_SPLIT_SCREEN){
		key_pos_x = m_posX+rc.x-20;
		key_pos_y = m_posY+rc.y-120;
	}
	else{
		key_pos_x = m_posX + (rc.x * m_scaleX) - 20;
		key_pos_y = m_posY + (rc.y * m_scaleY) - 100;
	}

	vita2d_draw_texture_part_scale(
			m_keyboard, 
			key_pos_x, // Pos x on vita screen
			key_pos_y, // Pos y on vita screen
			rc.x, // Pos x on keyboard bitmap
			rc.y, // Pos y on keyboard bitmap
			rc.width, 
			rc.height, 
			2, 
			2);
}

void VirtualKeyboard::initAnimation()
{
	// Check if keyboard sliding animation is disabled in the default conf file.
	// By default the animation is enabled and for now it can't be changed from settings menu.
	// If user really wants to disable it he/she has to manually modify to default configuration file.

	m_animation = true;
	const char* value = NULL;

	int ret = IniParser::getValueFromIni(DEF_CONF_FILE_PATH, 
										 INI_FILE_SEC_SETTINGS, 
										 INI_FILE_KEY_KEYBOARD_SLIDE, 
										 &value);

	if (ret == INI_PARSER_OK && value != NULL){
		if (!strcmp(value, "Disabled"))
			m_animation = false;

		delete[] value;
	}
}

bool VirtualKeyboard::isUpdated()
{
	return m_updated;
}

void VirtualKeyboard::setAnimation(bool animation)
{
	m_animation = animation;
}

void VirtualKeyboard::toggleVisibility()
{
	//PSV_DEBUG("VirtualKeyboard::toggleVisibility()");
	if (g_keyboardStatus == KEYBOARD_MOVING_UP || g_keyboardStatus == KEYBOARD_MOVING_DOWN)
		return;

	if (m_animation)
		g_keyboardStatus = (g_keyboardStatus == KEYBOARD_UP)? KEYBOARD_MOVING_DOWN: KEYBOARD_MOVING_UP;
	else
		g_keyboardStatus = (g_keyboardStatus == KEYBOARD_UP)? KEYBOARD_DOWN: KEYBOARD_UP;
}

int VirtualKeyboard::touchCoordinatesToMid(int x, int y)
{
	// Convert touch coordinates to keyboard matrix id.
	// TODO: Lookup table would be better here.

	// Touch screen coordinates are for some reason 1920x1088,
	// double the actual screen resolution. Convert for easier handling.
	x >>= 1;
	y >>= 1;
	
	if (g_keyboardMode == KEYBOARD_FULL_SCREEN){

		if (y >= 106 && y <= 166) // First row
		{
			if (x < 440){
				if (x >= 36 && x <= 81)
					return 113; // Arrow left
				if (x >= 86 && x <= 131)
					return 112; // 1
				if (x >= 137 && x <= 182)
					return 115; // 2
				if (x >= 188 && x <= 233)
					return 16; // 3
				if (x >= 239 && x <= 284)
					return 19; // 4
				if (x >= 290 && x <= 335)
					return 32; // 5
				if (x >= 341 && x <= 386)
					return 35; // 6
				if (x >= 392 && x <= 437)
					return 48; // 7	
			}
			else{
				if (x >= 442 && x <= 487)
					return 51; // 8
				if (x >= 493 && x <= 538)
					return 64; // 9
				if (x >= 544 && x <= 589)
					return 67; // 0
				if (x >= 595 && x <= 640)
					return 80; // +
				if (x >= 646 && x <= 691)
					return 83; // -
				if (x >= 697 && x <= 742)
					return 96; // Pound
				if (x >= 748 && x <= 793)
					return 99; // Home/Clr
				if (x >= 799 && x <= 844)
					return 0; // Del/Inst
				if (x >= 868 && x <= 950)
					return 4; // F1/F2
			}
		}
		else if (y >= 173 && y <= 233) // Second row
		{
			if (x < 468){
				if (x >= 36 && x <= 106)
					return 114; // CTRL
				if (x >= 113 && x <= 158)
					return 118; // Q
				if (x >=164 && x <= 209)
					return 17; // W
				if (x >= 215 && x <= 260)
					return 22; // E
				if (x >= 266 && x <= 311)
					return 33; // R
				if (x >=317 && x <= 362)
					return 38; // T
				if (x >= 368 && x <= 413)
					return 49; // Y
				if (x >= 419 && x <= 464)
					return 54; // U
			}
			else{
				if (x >= 470 && x <= 515)
					return 65; // I
				if (x >= 520 && x <= 565)
					return 70; // O
				if (x >= 571 && x <= 616)
					return 81; // P
				if (x >= 622 && x <= 667)
					return 86; // @
				if (x >= 673 && x <= 718)
					return 97; // *
				if (x >= 724 && x <= 769)
					return 102; // Arrow up
				if (x >= 775 && x <= 845)
					return 56; // Restore
				if (x >= 868 && x <= 950)
					return 5; // F3/F4
			}
		}
		else if (y >= 239 && y <= 299) // Third row
		{
			if (x < 427){
				if (x >= 23 && x <= 68)
					return 119; // Run/Stop
				if (x >= 74 && x <= 119)
					return 24; // Shift lock. This is 23 according to the matrix table but we have to differentiate it from left shift.
				if (x >= 124 && x <= 169)
					return 18; // A
				if (x >= 175 && x <= 220)
					return 21; // S
				if (x >= 226 && x <= 271)
					return 34; // D
				if (x >= 277 && x <= 322)
					return 37; // F
				if (x >= 328 && x <= 373)
					return 50; // G
				if (x >= 379 && x <= 424)
					return 53; // H
			}
			else{
				if (x >= 429 && x <= 474)
					return 66; // J
				if (x >= 480 && x <= 525)
					return 69; // K
				if (x >= 531 && x <= 576)
					return 82; // L
				if (x >= 582 && x <= 627)
					return 85; // :
				if (x >= 633 && x <= 678)
					return 98; // ;
				if (x >= 684 && x <= 729)
					return 101; // =
				if (x >= 735 && x <= 840)
					return 1; // Return
				if (x >= 868 && x <= 950)
					return 6; // F5/F6
			}
		}
		else if (y >= 305 && y <= 365) // Fourth row
		{
			if (x < 452){
				if (x >= 20 && x <= 68)
					return 117; // C=
				if (x >= 73 && x <= 143)
					return 23; // Left shift.
				if (x >= 148 && x <= 193)
					return 20; // Z
				if (x >= 199 && x <= 244)
					return 39; // X
				if (x >= 250 && x <= 295)
					return 36; // C
				if (x >= 301 && x <= 346)
					return 55; // V
				if (x >= 352 && x <= 397)
					return 52; // B
				if (x >= 403 && x <= 448)
					return 71; // N
			}
			else{
				if (x >= 453 && x <= 498)
					return 68; // M
				if (x >= 504 && x <= 549)
					return 87; // <
				if (x >= 555 && x <= 600)
					return 84; // >
				if (x >= 606 && x <= 651)
					return 103; // ?
				if (x >= 657 && x <= 729)
					return 100; // Right shift
				if (x >= 734 && x <= 779)
					return 7; // Cursor up/down
				if (x >= 785 && x <= 830)
					return 2; // Cursor left/right
				if (x >= 868 && x <= 950)
					return 3; // F7/F8
			}
		}
		else if (y >= 372 && y <= 432) // Space bar
		{
			if (x >= 165 && x <= 615)
				return 116;
		}
	}
	else if (g_keyboardMode == KEYBOARD_SPLIT_SCREEN){

		if (y >= 280 && y <= 339) // First row
		{
			if (x < 444){
				if (x >= 60 && x <= 121)
					return 113; // Arrow left
				if (x >= 122 && x <= 167)
					return 112; // 1
				if (x >= 168 && x <= 213)
					return 115; // 2
				if (x >= 214 && x <= 259)
					return 16; // 3
				if (x >= 260 && x <= 305)
					return 19; // 4
				if (x >= 306 && x <= 351)
					return 32; // 5
				if (x >= 352 && x <= 397)
					return 35; // 6
				if (x >= 398 && x <= 443)
					return 48; // 7	
			}
			else{
				if (x >= 444 && x <= 489)
					return 51; // 8
				if (x >= 490 && x <= 535)
					return 64; // 9
				if (x >= 536 && x <= 581)
					return 67; // 0
				if (x >= 582 && x <= 627)
					return 80; // +
				if (x >= 628 && x <= 673)
					return 83; // -
				if (x >= 674 && x <= 719)
					return 96; // Pound
				if (x >= 720 && x <= 765)
					return 99; // Home/Clr
				if (x >= 766 && x <= 811)
					return 0; // Del/Inst
				if (x >= 830 && x <= 900)
					return 4; // F1/F2
			}
		}
		else if (y >= 340 && y <= 385) // Second row
		{
			if (x < 468){
				if (x >= 60 && x <= 145)
					return 114; // CTRL
				if (x >= 146 && x <= 191)
					return 118; // Q
				if (x >=192 && x <= 237)
					return 17; // W
				if (x >= 238 && x <= 283)
					return 22; // E
				if (x >= 284 && x <= 329)
					return 33; // R
				if (x >=330 && x <= 375)
					return 38; // T
				if (x >= 376 && x <= 421)
					return 49; // Y
				if (x >= 422 && x <= 467)
					return 54; // U
			}
			else{
				if (x >=468  && x <= 513)
					return 65; // I
				if (x >= 514 && x <= 559)
					return 70; // O
				if (x >= 560 && x <= 605)
					return 81; // P
				if (x >= 606 && x <= 651)
					return 86; // @
				if (x >= 652 && x <= 697)
					return 97; // *
				if (x >= 698 && x <= 743)
					return 102; // Arrow up
				if (x >= 744 && x <= 811)
					return 56; // Restore
				if (x >= 830 && x <= 900)
					return 5; // F3/F4
			}
		}
		else if (y >= 386 && y <= 431) // Third row
		{
			if (x < 432){
				if (x >= 50 && x <= 109)
					return 119; // Run/Stop
				if (x >= 110 && x <= 155)
					return 24; // Shift lock. This is 23 according to the matrix table but we have to differentiate it from left shift.
				if (x >= 156 && x <= 201)
					return 18; // A
				if (x >= 202 && x <= 247)
					return 21; // S
				if (x >= 248 && x <= 293)
					return 34; // D
				if (x >= 294 && x <= 339)
					return 37; // F
				if (x >= 340 && x <= 385)
					return 50; // G
				if (x >= 386 && x <= 431)
					return 53; // H
			}
			else{
				if (x >= 432 && x <= 477)
					return 66; // J
				if (x >= 478 && x <= 523)
					return 69; // K
				if (x >= 524 && x <= 569)
					return 82; // L
				if (x >= 570 && x <= 615)
					return 85; // :
				if (x >= 616 && x <= 661)
					return 98; // ;
				if (x >= 662 && x <= 707)
					return 101; // =
				if (x >= 708 && x <= 813)
					return 1; // Return
				if (x >= 830 && x <= 900)
					return 6; // F5/F6
			}
		}
		else if (y >= 432 && y <= 477) // Fourth row
		{
			if (x < 454){
				if (x >= 50 && x <= 109)
					return 117; // C=
				if (x >= 110 && x <= 177)
					return 23; // Left shift
				if (x >= 178 && x <= 223)
					return 20; // Z
				if (x >= 224 && x <= 269)
					return 39; // X
				if (x >= 270 && x <= 315)
					return 36; // C
				if (x >= 316 && x <= 361)
					return 55; // V
				if (x >= 362 && x <= 407)
					return 52; // B
				if (x >= 408 && x <= 453)
					return 71; // N
			}
			else{
				if (x >= 454 && x <= 499)
					return 68; // M
				if (x >= 500 && x <= 545)
					return 87; // <
				if (x >= 546 && x <= 591)
					return 84; // >
				if (x >= 592 && x <= 637)
					return 103; // ?
				if (x >= 638 && x <= 707)
					return 100; // Right shift
				if (x >= 708 && x <= 753)
					return 7; // Cursor up/down
				if (x >=754  && x <= 813)
					return 2; // Cursor left/right
				if (x >= 830 && x <= 900)
					return 3; // F7/F8
			}
		}
		else if (y >= 476 && y <= 490) // Extension
		{
			// Extend C= and shifts to make easier to press them.
			if (x >= 50 && x <= 106)
				return 117; // C=
			if (x >= 112 && x <= 172)
				return 23; // Left shift
			if (x >= 640 && x <= 704)
				return 100; // Right shift
		}
		else if (y >= 491 && y <= 540) // Space bar
		{
			if (x >= 194 && x <= 600) 
				return 116;
		}
	}

	return -1; // Unknown
}

void VirtualKeyboard::midToKeyboardCoordinates(int mid, RectCoordinates* tc)
{
	switch (mid){
	// First row.
	case 113: tc->y=20; tc->x=33; tc->width=40; tc->height=40; break;
	case 112: tc->y=20; tc->x=79; tc->width=40; tc->height=40; break;
	case 115: tc->y=20; tc->x=125; tc->width=40; tc->height=40; break;
	case 16:  tc->y=20; tc->x=171; tc->width=40; tc->height=40; break;
	case 19:  tc->y=20; tc->x=217; tc->width=40; tc->height=40; break;
	case 32:  tc->y=20; tc->x=263; tc->width=40; tc->height=40; break;
	case 35:  tc->y=20; tc->x=309; tc->width=40; tc->height=40; break;
	case 48:  tc->y=20; tc->x=355; tc->width=40; tc->height=40; break;
	case 51:  tc->y=20; tc->x=401; tc->width=40; tc->height=40; break;
	case 64:  tc->y=20; tc->x=447; tc->width=40; tc->height=40; break;
	case 67:  tc->y=20; tc->x=493; tc->width=40; tc->height=40; break;
	case 80:  tc->y=20; tc->x=539; tc->width=40; tc->height=40; break;
	case 83:  tc->y=20; tc->x=585; tc->width=40; tc->height=40; break;
	case 96:  tc->y=20; tc->x=631; tc->width=40; tc->height=40; break;
	case 99:  tc->y=20; tc->x=677; tc->width=40; tc->height=40; break;
	case 0:   tc->y=20; tc->x=723; tc->width=40; tc->height=40; break;
	case 4:   tc->y=20; tc->x=787; tc->width=60; tc->height=40; break;
	// Second row.
	case 114: tc->y=66; tc->x=33; tc->width=60; tc->height=40; break;
	case 118: tc->y=66; tc->x=103; tc->width=40; tc->height=40; break; 
	case 17:  tc->y=66; tc->x=149; tc->width=40; tc->height=40; break; 
	case 22:  tc->y=66; tc->x=195; tc->width=40; tc->height=40; break;
	case 33:  tc->y=66; tc->x=241; tc->width=40; tc->height=40; break;
	case 38:  tc->y=66; tc->x=287; tc->width=40; tc->height=40; break;
	case 49:  tc->y=66; tc->x=333; tc->width=40; tc->height=40; break;
	case 54:  tc->y=66; tc->x=379; tc->width=40; tc->height=40; break;
	case 65:  tc->y=66; tc->x=425; tc->width=40; tc->height=40; break;
	case 70:  tc->y=66; tc->x=471; tc->width=40; tc->height=40; break;
	case 81:  tc->y=66; tc->x=517; tc->width=40; tc->height=40; break;
	case 86:  tc->y=66; tc->x=563; tc->width=40; tc->height=40; break;
	case 97:  tc->y=66; tc->x=609; tc->width=40; tc->height=40; break;
	case 102: tc->y=66; tc->x=655; tc->width=40; tc->height=40; break;
	case 56:  tc->y=66; tc->x=701; tc->width=63; tc->height=40; break;
	case 5:   tc->y=66; tc->x=787; tc->width=60; tc->height=40; break;
	// Third row.
	case 119: tc->y=112; tc->x=22; tc->width=40; tc->height=40; break;
	case 24:  tc->y=112; tc->x=68; tc->width=40; tc->height=40; break; 
	case 18:  tc->y=112; tc->x=114; tc->width=40; tc->height=40; break; 
	case 21:  tc->y=112; tc->x=160; tc->width=40; tc->height=40; break;
	case 34:  tc->y=112; tc->x=206; tc->width=40; tc->height=40; break;
	case 37:  tc->y=112; tc->x=252; tc->width=40; tc->height=40; break;
	case 50:  tc->y=112; tc->x=298; tc->width=40; tc->height=40; break;
	case 53:  tc->y=112; tc->x=344; tc->width=40; tc->height=40; break;
	case 66:  tc->y=112; tc->x=390; tc->width=40; tc->height=40; break;
	case 69:  tc->y=112; tc->x=436; tc->width=40; tc->height=40; break;
	case 82:  tc->y=112; tc->x=482; tc->width=40; tc->height=40; break;
	case 85:  tc->y=112; tc->x=528; tc->width=40; tc->height=40; break;
	case 98:  tc->y=112; tc->x=574; tc->width=40; tc->height=40; break;
	case 101: tc->y=112; tc->x=620; tc->width=40; tc->height=40; break;
	case 1:   tc->y=112; tc->x=666; tc->width=85; tc->height=40; break;
	case 6:   tc->y=112; tc->x=787; tc->width=60; tc->height=40; break;
	// Fourth row.
	case 117: tc->y=158; tc->x=22; tc->width=40; tc->height=40; break;
	case 23:  tc->y=158; tc->x=68; tc->width=62; tc->height=40; break; 
	case 20:  tc->y=158; tc->x=136; tc->width=40; tc->height=40; break; 
	case 39:  tc->y=158; tc->x=182; tc->width=40; tc->height=40; break;
	case 36:  tc->y=158; tc->x=228; tc->width=40; tc->height=40; break;
	case 55:  tc->y=158; tc->x=274; tc->width=40; tc->height=40; break;
	case 52:  tc->y=158; tc->x=320; tc->width=40; tc->height=40; break;
	case 71:  tc->y=158; tc->x=366; tc->width=40; tc->height=40; break;
	case 68:  tc->y=158; tc->x=412; tc->width=40; tc->height=40; break;
	case 87:  tc->y=158; tc->x=458; tc->width=40; tc->height=40; break;
	case 84:  tc->y=158; tc->x=504; tc->width=40; tc->height=40; break;
	case 103: tc->y=158; tc->x=550; tc->width=40; tc->height=40; break;
	case 100: tc->y=158; tc->x=596; tc->width=64; tc->height=40; break;
	case 7:   tc->y=158; tc->x=666; tc->width=40; tc->height=40; break;
	case 2:   tc->y=158; tc->x=712; tc->width=40; tc->height=40; break;
	case 3:   tc->y=158; tc->x=787; tc->width=60; tc->height=40; break;
	}
}