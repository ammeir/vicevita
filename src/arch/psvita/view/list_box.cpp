
/* list_box.cpp: Implementation of a list box gui element.

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

#include "list_box.h"
#include "scroll_bar.h"
#include "texter.h"
#include "iRenderable.h"
#include "resources.h"
#include "app_defs.h"
#include "debug_psv.h"
#include <string.h> // memcpy

#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <vita2d.h>


#define MAX_ENTRIES 17
#define FONT_Y_SPACE 22
#define SCROLL_BAR_WIDTH 8

ListBox::ListBox()
{
}

ListBox::~ListBox()
{	
}

void ListBox::init(int x, int y, int width, int height, int frame_size, ScrollBarSide scrollbar_side, int alignment, IRenderable* background)
{
	m_posX = x;
	m_posY = y;
	m_width = width;
	m_height = height;
	m_highlight = 0;
	m_borderTop = 0;
	m_borderBottom = height / FONT_Y_SPACE - 1;
	m_backgroundColor = LIGHT_GREY;
	m_frameColor = DARK_GREY;
	m_textColor = BLACK;
	m_spacing = FONT_Y_SPACE;
	m_selected = false;
	m_visible = true;
	m_frame = true;
	m_textSize = 24;
	m_showArrow = false;
	m_frameSize = frame_size;
	m_scrollBarSide = scrollbar_side;
	m_indentSize = 10;
	m_fullHLBar = false;
	m_alignment = alignment;
	m_background = background;

	if (m_scrollBarSide == SCROLL_BAR_RIGHT)
		m_scrollBar.init((m_posX + m_width - m_frameSize - SCROLL_BAR_WIDTH), m_posY + m_frameSize, SCROLL_BAR_WIDTH, m_height-m_frameSize*2);
	else
		m_scrollBar.init(m_posX + m_frameSize, m_posY + m_frameSize, SCROLL_BAR_WIDTH, m_height-m_frameSize*2);

	m_scrollBar.setBackColor(GREY);
	m_scrollBar.setBarColor(ROYAL_BLUE);
}

void ListBox::doModal()
{
	m_selected = false;
	m_exitButton = 0;
	scanCyclic();
}

void ListBox::render()
{
	show(false);
}

void ListBox::addItems(const char* menuEntries[], int size)
{
	for(int i=0; i<size; ++i)
	{
		m_list.push_back(string(menuEntries[i]));
	}

	if (!m_list.empty()){
		m_textHeight = txtr_get_text_height(m_list[0].c_str(), m_textSize);
		m_maxLinesInView = (m_height) / m_spacing;
		m_borderBottom = m_maxLinesInView - 1;
		m_scrollBar.setListSize(m_list.size(), m_maxLinesInView);
	}
}

void ListBox::clear()
{
	m_list.clear();
}

void ListBox::show(bool standAlone)
{
	if (standAlone){
		vita2d_start_drawing();
		vita2d_clear_screen();
	}

	int line_color;
	int start = m_borderTop;
	int end = (m_list.size() > m_maxLinesInView)? m_borderBottom+1: m_list.size();
	int y = m_posY+m_frameSize+m_textHeight+3;
	int frame_add_x = 0;
	int highlight_rect_w;
	int highlight_rect_h; 

	// Draw background if available
	if (m_background)
		m_background->render();

	// Draw frame
	if (m_frame){
		vita2d_draw_rectangle(m_posX, m_posY, m_width, m_frameSize, m_frameColor); // Top 
		vita2d_draw_rectangle(m_posX, m_posY+m_height-m_frameSize, m_width, m_frameSize, m_frameColor); // Bottom
		vita2d_draw_rectangle(m_posX, m_posY, m_frameSize, m_height, m_frameColor); // Left
		vita2d_draw_rectangle(m_posX+m_width-m_frameSize, m_posY, m_frameSize, m_height, m_frameColor); // Right                                                         
		// Draw background color rectangle
		vita2d_draw_rectangle(m_posX+m_frameSize, m_posY+m_frameSize, m_width-m_frameSize*2, m_height-m_frameSize*2, m_backgroundColor);

		frame_add_x = 8;
	}

	// Draw list items
	for (int i=start; i<end; ++i){
		line_color = m_textColor;
		if (i == m_highlight){
			line_color = WHITE;
			highlight_rect_w = txtr_get_text_width(m_list[i].c_str(), m_textSize) + 6;
			highlight_rect_h = m_textHeight + 4;

			// Draw highlight rectangle
			if (m_fullHLBar){
				// Listbox width size rectangle
				vita2d_draw_rectangle(m_posX+m_frameSize+3, y-highlight_rect_h+5, m_width-(2*m_frameSize+SCROLL_BAR_WIDTH+6), highlight_rect_h, ROYAL_BLUE);
			}else{
				// Text width size rectangle
				vita2d_draw_rectangle(m_posX+m_indentSize-3, y-highlight_rect_h+5, highlight_rect_w, highlight_rect_h, ROYAL_BLUE);
			}

			if (m_showArrow)
				txtr_draw_text(m_posX+m_indentSize+highlight_rect_w, y, line_color, ">");
		}

		if (m_alignment == 3){// Center
			// Center the text to area excluding frames and scrollbar
			int text_width = txtr_get_text_width(m_list[i].c_str(), m_textSize);
			if (text_width < m_width){
				// Text fits in menu item. Calculate new indent size to align the text center
				m_indentSize = ((m_width-(2*m_frameSize+SCROLL_BAR_WIDTH+6)) - text_width) / 2;
				m_indentSize += m_frameSize+3; // add default intendation
			}
		}

		txtr_draw_text(m_posX+m_indentSize, y, line_color, m_list[i].c_str());
		y = y + m_spacing;
	}

	if (m_list.size() > m_maxLinesInView)
		m_scrollBar.render();

	if (standAlone){
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers(); 
	}
}

void ListBox::navigateUp()
{
	if (m_highlight > 0){
		if (m_highlight-- == m_borderTop){
			m_borderTop--;
			m_borderBottom--;
			m_scrollBar.scrollUp();
		}

		show(true);
	}
}

void ListBox::navigateDown()
{
	if (m_highlight < (m_list.size()-1)){
		if (m_highlight++ == m_borderBottom){
			m_borderBottom++;
			m_borderTop++;
			m_scrollBar.scrollDown();
		}

		show(true);
	}
}

void ListBox::buttonReleased(int button)
{
	if (isExit(button))
		Navigator::m_running = false;
}

bool ListBox::isExit(int buttons)
{
	if(buttons == SCE_CTRL_CROSS){ // Menu selection
		m_exitButton = buttons;
		m_selected = true;
		return true;
	}
	else if (buttons == SCE_CTRL_LEFT){ // Leave menu
		m_exitButton = buttons;
		return true;
	}
	
	return false;
}

string ListBox::getSelected()
{
	string ret;

	if (m_selected){
		ret = m_list[m_highlight];
	}

	return ret;
}

unsigned int ListBox::getExitButton()
{
	return m_exitButton;
}

void ListBox::setVisibility(bool visible)
{
	m_visible = visible;
}

bool ListBox::isVisible()
{
	return m_visible;
}

void ListBox::setBackgroundColor(int color)
{
	m_backgroundColor = color;
}

void ListBox::setFrameColor(int color)
{
	m_frameColor = color;
}

void ListBox::setTextColor(int color)
{
	m_textColor = color;
}

void ListBox::setSpacing(int val)
{
	m_spacing = val;
}

void ListBox::showFrame(bool val)
{
	m_frame = val;
}

void ListBox::setTextSize(int val)
{
	m_textSize = val;

	if (!m_list.empty()){
		m_textHeight = txtr_get_text_height(m_list[0].c_str(), m_textSize);
		m_maxLinesInView = (m_height) / m_spacing;
		m_borderBottom = m_maxLinesInView - 1;
		m_scrollBar.setListSize(m_list.size(), m_maxLinesInView);
	}
}

void ListBox::showArrowOnHighlight(bool val)
{
	m_showArrow = val;
}

void ListBox::setIndentSize(int val)
{
	m_indentSize = val;
}

void ListBox::setFullHighLightBar(bool val)
{
	m_fullHLBar = val;
}

void ListBox::setHighlighItem(const char* name)
{
	if (!name)
		return;

	int ind = 0;
	for (vector<string>::iterator it = m_list.begin(); it != m_list.end(); ++it){
		if (*it == name){
			m_highlight = ind;
			break;
		}
		ind++;
	}
}


