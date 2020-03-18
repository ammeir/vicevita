
/* dialog_box.cpp: Implementation of a dialog box gui element.

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

#include "dialog_box.h"
#include "texter.h"
#include "iRenderable.h"
#include "resources.h"
#include "debug_psv.h"

#include <vector>
#include <string.h> // memcpy
#include <vita2d.h>
#include <psp2/ctrl.h>

#define BLACK   RGBA8(0, 0, 0, 255)
#define WHITE	RGBA8(255, 255, 255, 255)
#define YELLOW  RGBA8(192,192,0,255)


MsgDialog::MsgDialog()
{
	m_fb_tex = NULL;
	m_img = NULL;
	m_img_btn_confirm;
	m_img_btn_cancel;
	m_background = NULL;
}

MsgDialog::~MsgDialog()
{
	if (m_fb_tex)
		vita2d_free_texture(m_fb_tex);
	if (m_img)
		vita2d_free_texture(m_img);
	if (m_img_btn_confirm)
		vita2d_free_texture(m_img_btn_confirm);
	if (m_img_btn_cancel)
		vita2d_free_texture(m_img_btn_cancel);
}

void MsgDialog::init(MsgDialogParams& params)
{
	m_background = params.background;
	m_button_type = params.buttonType;
	m_msg = params.msg;
	m_width = 60; // To make the box little bigger than text
	m_height = 125;

	if (m_button_type == MSG_DIALOG_BUTTON_TYPE_OK_CANCEL){
		m_btn1_text = params.buttonParam.text1;
		m_btn2_text = params.buttonParam.text2;

		int space_between_btns = 25;
		int btn1_text_w = txtr_get_text_width(m_btn1_text.c_str(), 22);
		int btn2_text_w = txtr_get_text_width(m_btn2_text.c_str(), 22);

		int msg_width = txtr_get_text_width(m_msg.c_str(), 26);
		int btn_row_width = btn1_text_w + btn2_text_w +
							30*2 + // button texture width
							space_between_btns;

		m_width += (msg_width > btn_row_width)? msg_width: btn_row_width;

		m_box_posX = (960 - m_width) / 2;
		m_box_posY = (544 - m_height) / 2; 

		m_title_posX = m_box_posX + (m_width - msg_width) / 2 ;
		m_title_posY = m_box_posY + 50;
		m_btn1_posX = m_box_posX + (m_width - btn_row_width) / 2;
		m_btn2_posX = m_btn1_posX + 30 + btn1_text_w + space_between_btns;
		m_btn_posY = m_box_posY + 50 + 30;
	}
	else if (m_button_type == MSG_DIALOG_BUTTON_TYPE_OK){
		m_btn1_text = params.buttonParam.text1;

		int btn1_text_w = txtr_get_text_width(m_btn1_text.c_str(), 22);

		int msg_width = txtr_get_text_width(m_msg.c_str(), 26);
		int btn_row_width = btn1_text_w + 
							30; // button texture width
						
		m_width += (msg_width > btn_row_width)? msg_width: btn_row_width;

		m_box_posX = (960 - m_width) / 2;
		m_box_posY = (544 - m_height) / 2; 

		m_title_posX = m_box_posX + (m_width - msg_width) / 2 ;
		m_title_posY = m_box_posY + 50;
		m_btn1_posX = m_box_posX + (m_width - btn_row_width) / 2;
		m_btn_posY = m_box_posY + 50 + 30;
	}
	else if (m_button_type == MSG_DIALOG_BUTTON_TYPE_NONE){
		
		if (!m_msg.empty()){
			int msg_width = txtr_get_text_width(m_msg.c_str(), 26);
	
			m_width += msg_width;
			m_width += 20; // Box looks better little wider
			m_height = 100;

			m_box_posX = (960 - m_width) / 2;
			m_box_posY = (544 - m_height) / 2; 

			m_title_posX = m_box_posX + (m_width - msg_width) / 2 ;
			m_title_posY = m_box_posY + 60;
		}
		else{
			// Message is an image
			m_img = vita2d_load_PNG_buffer(params.img);
			m_width = vita2d_texture_get_width(m_img);
			m_height = vita2d_texture_get_height(m_img);
			m_box_posX = (960 - m_width) / 2;
			m_box_posY = (544 - m_height) / 2; 
		}
	}

	// Allocate memory for the front buffer copy. This is used as a background texture for the dialog.
	m_fb_tex = vita2d_create_empty_texture_format(1024, 544, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
	m_pfb_tex = (unsigned char*)vita2d_texture_get_datap(m_fb_tex);
	// Dialog buttons.
	m_img_btn_confirm = vita2d_load_PNG_buffer(img_ctrl_btn_cross_black);
	m_img_btn_cancel = vita2d_load_PNG_buffer(img_ctrl_btn_circle_black);
}

void MsgDialog::doModal(MsgDialogResult& res)
{
	show();
	scanCyclic();	

	res.buttonId = m_exitBtnId;
}

bool MsgDialog::isExit(int button)
{
	if (button == SCE_CTRL_CIRCLE){
		m_exitBtnId = MSG_DIALOG_BUTTON_ID_NO;
		return true;
	}
	else if (button == SCE_CTRL_CROSS){
		m_exitBtnId = MSG_DIALOG_BUTTON_ID_OK;
		return true;
	}

	return false;
}

void MsgDialog::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();
	
	// Render the background for the msg box
	if (m_background)
		m_background->render();
	else
		renderFrontBuffer(); // This works but doesn't look great. Needs more investigation.

	// Dialog box
	vita2d_draw_rectangle(m_box_posX, m_box_posY, m_width, m_height, BLACK);

	// Dialog box frame
	int frame_size = 2;
	vita2d_draw_rectangle(m_box_posX-frame_size, m_box_posY-frame_size, m_width+(frame_size*2), frame_size, WHITE); // Top 
	vita2d_draw_rectangle(m_box_posX-frame_size, m_box_posY+m_height, m_width+(frame_size*2), frame_size, WHITE); // Bottom
	vita2d_draw_rectangle(m_box_posX-frame_size, m_box_posY, frame_size, m_height, WHITE); // Left
	vita2d_draw_rectangle(m_box_posX+m_width, m_box_posY, frame_size, m_height, WHITE); // Right     
	
	if (m_button_type == MSG_DIALOG_BUTTON_TYPE_OK_CANCEL){
		// Title message
		txtr_draw_text(m_title_posX, m_title_posY, YELLOW, m_msg.c_str());
	
		// Buttons
		vita2d_draw_texture(m_img_btn_confirm, m_btn1_posX, m_btn_posY); // Cross button
		txtr_draw_text(m_btn1_posX+25, m_btn_posY+18, WHITE, m_btn1_text.c_str());

		vita2d_draw_texture(m_img_btn_cancel, m_btn2_posX, m_btn_posY); // Circle button
		txtr_draw_text(m_btn2_posX+25, m_btn_posY+18, WHITE, m_btn2_text.c_str());
	}
	else if(m_button_type == MSG_DIALOG_BUTTON_TYPE_OK){
		// Title message
		txtr_draw_text(m_title_posX, m_title_posY, YELLOW, m_msg.c_str());
		// Buttons
		vita2d_draw_texture(m_img_btn_confirm, m_btn1_posX, m_btn_posY); // Cross button
		txtr_draw_text(m_btn1_posX+25, m_btn_posY+18, WHITE, m_btn1_text.c_str());
	}
	else if (m_button_type == MSG_DIALOG_BUTTON_TYPE_NONE){
		if (m_img){
			// Image
			vita2d_draw_texture(m_img, m_box_posX, m_box_posY); 	
		}
		else{
			// Title message
			txtr_draw_text(m_title_posX, m_title_posY, YELLOW, m_msg.c_str());
		}
	}

	vita2d_end_drawing(); // This actions all pending draws. Nothing is still visible.
	vita2d_swap_buffers(); // This will make all draws visible
	vita2d_wait_rendering_done();
}

void MsgDialog::renderFrontBuffer()
{
	// Get front buffer
	unsigned char* fb = (unsigned char*)vita2d_get_current_fb();
	
	// Copy to texture
	memcpy(m_pfb_tex, fb, 1024*544*4);

	vita2d_draw_texture(m_fb_tex, 0, 0);
}
