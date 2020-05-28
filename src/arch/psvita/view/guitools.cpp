
/* guitools.cpp: Usefull GUI utilities.

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

#include "guitools.h"
#include "file_explorer.h"
#include "dialog_box.h"
#include "list_box.h"
#include "texter.h"
#include "resources.h"
#include "debug_psv.h"
#include <vita2d.h>


string gtShowFileBrowser(const char* path, int hlIndex, int btIndex, float sbPosY, const char** filter)
{
	FileExplorer fileExp;
	fileExp.init(path, hlIndex, btIndex, sbPosY, filter);
	return  fileExp.doModal();
}

string gtShowListBox(int x, 
					int y,
					int width,
					int height,
					const char** values, 
					int size, 
					IRenderable* background, 
					const char* highlightName, 
					int fontSize)
{
	
	/* Show listbox with default properties */

	int frame_size = 5;

	// Calculate box size if dimensions are omitted.
	if (!width){
		int h;
		getListBoxDimensions(values, size, fontSize, &width, &h);
		width += 2*frame_size+10+10; // add frame size, scrollbar size and intendation
	}
	if (!height){
		int w;
		getListBoxDimensions(values, size, fontSize, &w, &height);
		height += (2*frame_size)+(5*(size-1))+(2*5); // add frame size, space between lines, vertical intendation
	}

	// Maximum height.
	if (height > 430) 
		height = 430; 

	// Prevent the box getting over the edges.
	if (y + height > 500)
		y -= (y + height - 500);
	if (x + width > 960)
		x -= (x + width - 960);

	
	ListBox box;
	box.init(x, y, width, height, frame_size, SCROLL_BAR_RIGHT, 1, background);
	box.setFullHighLightBar(true);
	box.addItems(values, size);
	box.setHighlighItem(highlightName);
	box.show(true);
	box.doModal();

	return box.getSelected();
}

string gtShowListBoxEx(int x, 
					int y,
					int width,
					int height,
					const char** values, 
					int size, 
					IRenderable* background, 
					const char* highlightName, 
					int spacing, // Space between rows
					int fontSize,
					int textColor,
					int backColor,
					int frameColor)
{
	
	/* Show listbox with customized properties */

	int frame_size = 5;

	// Calculate box size if dimensions are omitted
	if (!width){
		int h;
		getListBoxDimensions(values, size, fontSize, &width, &h);
		width += 2*frame_size+10+10; // add frame size, scrollbar size and intendation
	}
	if (!height){
		int w;
		getListBoxDimensions(values, size, fontSize, &w, &height);
		height += (2*frame_size)+(5*(size-1))+(2*5); // add frame size, space between lines, vertical intendation
	}

	// Maximum height.
	if (height > 430) 
		height = 430; 

	// Prevent the box getting over the edges.
	if (y + height > 500)
		y -= (y + height - 500);
	if (x + width > 960)
		x -= (x + width - 960);

	ListBox box;
	box.init(x, y, width, height, frame_size, SCROLL_BAR_RIGHT, 1, background);
	box.setFullHighLightBar(true);
	box.addItems(values, size);
	box.setBackgroundColor(backColor);
	box.setTextColor(textColor);
	box.setFrameColor(frameColor);
	box.setTextSize(fontSize);
	box.setSpacing(spacing);
	box.setHighlighItem(highlightName);
	box.show(true);
	box.doModal();

	return box.getSelected();
}

void gtShowMsgBoxOk(const char* msg, IRenderable* background)
{
	MsgDialogParams params;
	MsgDialogResult result;

	params.buttonType = MSG_DIALOG_BUTTON_TYPE_OK;
	params.msg = msg;
	params.buttonParam.text1 = "Ok";
	params.background = background;

	MsgDialog dlg;
	dlg.init(params);
	dlg.doModal(result);
}

bool gtShowMsgBoxOkCancel(const char* msg, IRenderable* background)
{
	bool ret = false;
	MsgDialogParams params;
	MsgDialogResult result;

	params.buttonType = MSG_DIALOG_BUTTON_TYPE_OK_CANCEL;
	params.msg = msg;
	params.buttonParam.text1 = "Ok";
	params.buttonParam.text2 = "Cancel";
	params.background = background;

	MsgDialog dlg;
	dlg.init(params);
	dlg.doModal(result);

	if (result.buttonId == MSG_DIALOG_BUTTON_ID_OK || result.buttonId == MSG_DIALOG_BUTTON_ID_YES)
		ret = true;

	return ret;
}

void gtShowMsgBoxNoBtn(const char* msg, IRenderable* background)
{
	MsgDialogParams params;
	MsgDialogResult result;

	params.buttonType = MSG_DIALOG_BUTTON_TYPE_NONE;
	params.msg = msg;
	params.background = background;

	MsgDialog dlg;
	dlg.init(params);
	dlg.show();
}

void gtShowMsgBoxPng(char* img, IRenderable* background)
{
	MsgDialogParams params;
	MsgDialogResult result;

	params.buttonType = MSG_DIALOG_BUTTON_TYPE_NONE;
	params.img = img;
	params.background = background;

	MsgDialog dlg;
	dlg.init(params);
	dlg.show();
}

void getListBoxDimensions(const char** values, int size, int fontSize, int* width, int* height)
{
	// Calculate listbox width and height.
	*width = 0;
	*height = 0;
	for (int i=0; i<size; i++){
		int tmp = txtr_get_text_width(values[i], fontSize);
		*height += txtr_get_text_height(values[i], fontSize);
		
		if (tmp > *width)
			*width = tmp;
	}
}
