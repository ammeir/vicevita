
/* guitools.h: Usefull GUI utilities.

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

#ifndef GUITOOLS_H
#define GUITOOLS_H

#include "iRenderable.h"
#include <string>

using std::string;

static void	getListBoxDimensions(const char** values, 
								 int size, 
								 int fontSize, 
								 int* width, 
								 int* height);
				
string		gtShowFileBrowser(const char* path, 
							int hlIndex = 0, 
							int btIndex = 0, 
							float sbPosY = 0, 
							const char** filter = NULL, 
							int fsize = 0);

string		gtShowListBox(int x, 
						int y,
						int width,
						int height,
						const char** values, 
						int valuesSize,
						IRenderable* background,
						const char* highlightName = NULL, // Value to show highlighted
						int fontSize = 0); 
					
string		gtShowListBoxEx(int x, 
							int y,
							int width,
							int height,
							const char** values, 
							int valuesSize,
							IRenderable* background,
							const char* highlightName, 
							int spacing, // Space between rows
							int fontSize,
							int textColor,
							int backColor,
							int frameColor);

void		gtShowMsgBoxOk(const char* msg, IRenderable* background = NULL);
bool		gtShowMsgBoxOkCancel(const char* msg, IRenderable* background = NULL);
void		gtShowMsgBoxNoBtn(const char* msg, IRenderable* background = NULL);
void		gtShowMsgBoxPng(char* img, IRenderable* background = NULL);

#endif