
/* texter.h: Little wrapper to simplify text drawing.

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

#ifndef TEXTER_H
#define TEXTER_H

void txtr_free();
void txtr_load_font_file(const char* path);		
void txtr_load_font_mem(const unsigned char* buf, int size);	
void txtr_draw_text(int x, int y, int color, const char* text, float font_size = 1.0);
int txtr_get_text_width(const char* text, float font_size = 1.0);
int txtr_get_text_height(const char* text, float font_size = 1.0);

#endif