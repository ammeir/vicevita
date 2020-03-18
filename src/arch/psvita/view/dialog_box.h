
/* dialog_box.h: Implementation of a dialog box gui element.

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

#ifndef DIALOG_H
#define DIALOG_H

#include "navigator.h"
#include <string>

using std::string;

typedef enum MsgDialogMode {
	MSG_DIALOG_MODE_USER_MSG     = 0,
	MSG_DIALOG_MODE_SYSTEM_MSG   = 1,
	MSG_DIALOG_MODE_ERROR_CODE   = 2
} MsgDialogMode;

typedef enum MsgDialogButtonType {
	MSG_DIALOG_BUTTON_TYPE_NONE		   = 0, // No buttons        
	MSG_DIALOG_BUTTON_TYPE_OK          = 1,
	MSG_DIALOG_BUTTON_TYPE_YESNO       = 2,
	MSG_DIALOG_BUTTON_TYPE_OK_CANCEL   = 3
} MsgDialogButtonType;

typedef enum MsgDialogButtonId {
	MSG_DIALOG_BUTTON_ID_NONE		= 0,
	MSG_DIALOG_BUTTON_ID_OK         = 1,
	MSG_DIALOG_BUTTON_ID_YES        = 2,
	MSG_DIALOG_BUTTON_ID_NO         = 3
} MsgDialogButtonId;


typedef struct MsgDialogButtonsParam {
	string text1;         // Text of the first button
	int fontSize1;        // Font size of the first button 
	string text2;         // Text of the second button
	int fontSize2;        // Font size of the second button 
	string text3;         // Text of the third button
	int fontSize3;        // Font size of the third button 
} MsgDialogButtonsParam;

class IRenderable;
typedef struct MsgDialogParams {
	MsgDialogButtonType buttonType; // Type of button set (one of MsgDialogButtonType)
	string msg;						// Displayed message
	char*  img;						// Displayed image
	int    img_w;
	int    img_h;
	MsgDialogButtonsParam buttonParam;	// Buttons parameters
	IRenderable* background;			// Interface to render the background
} MsgDialogParams;

typedef struct MsgDialogResult {
	int mode;                       // Mode of function (one of MsgDialogMode)
	int result;                     // Result of executing function
	int buttonId;                   // Id of button user selected (one of MsgDialogButtonId)
} MsgDialogResult;


class vita2d_texture;
class MsgDialog : public Navigator
{

private:
	vita2d_texture*		m_img;
	vita2d_texture*		m_img_btn_confirm;
	vita2d_texture*		m_img_btn_cancel;
	string				m_msg;
	int					m_exitBtnId;
	int					m_box_posX;
	int					m_box_posY;
	int					m_title_posX;
	int					m_title_posY;
	int					m_btn1_posX;
	int					m_btn2_posX;
	int					m_btn_posY;
	int					m_width;
	int					m_height;
	MsgDialogButtonType	m_button_type;
	string				m_btn1_text;
	string				m_btn2_text;
	IRenderable*		m_background;
	vita2d_texture*		m_fb_tex;
	unsigned char*		m_pfb_tex;

	// Navigator interface implementations
	bool				isExit(int buttons); 
	void				renderFrontBuffer();

public:
						MsgDialog();
						~MsgDialog();

	void				doModal(MsgDialogResult& res);
	void				init(MsgDialogParams& params);
	void				show();

};

#endif
