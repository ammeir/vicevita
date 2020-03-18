
/* scroll_bar.cpp: Implementation of a window scroll bar gui element.

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

#include "scroll_bar.h"
#include "app_defs.h"
#include "debug_psv.h"
#include <vita2d.h>


ScrollBar::ScrollBar()
{

}

ScrollBar::~ScrollBar()
{

}

void ScrollBar::init(int x, int y, int width, int height)
{
	m_posX = x;
	m_posY = y;
	m_scrollerPosY = y;
	m_scrollerPosYMin = y;
	m_width = width;
	m_height = height;
	m_barColor = GREY;
	m_backColor = LIGHT_GREY;
	m_listSize = 0;
	m_scrollerHeight = 0;
	m_scrollerPosYMax = 0;
	m_scrollStep = 0.0f;
}

void ScrollBar::scrollUp()
{
	if (m_scrollerPosY > m_scrollerPosYMin){
		m_scrollerPosY -= m_scrollStep;
	}
}

void ScrollBar::scrollDown()
{
	if (m_scrollerPosY < m_scrollerPosYMax){
		m_scrollerPosY += m_scrollStep;
	}
}

void ScrollBar::render()
{
	// Draw background rectangle
	vita2d_draw_rectangle(m_posX, m_posY, m_width, m_height, m_backColor);
	
	// Draw scroller rectangle
	vita2d_draw_rectangle(m_posX, m_scrollerPosY, m_width, m_scrollerHeight, m_barColor);
}

// This function must be called after each directory entry
void ScrollBar::setListSize(int size, int max_entries_in_view)
{
	m_listSize = size;
	m_scrollerHeight = (float)max_entries_in_view / m_listSize * m_height;
	m_scrollerPosYMax = m_posY + (m_height - m_scrollerHeight);
	m_scrollStep = ((float)m_height - m_scrollerHeight) / ((float)m_listSize - max_entries_in_view);
}

void ScrollBar::setBarColor(int color)
{
	m_barColor = color;
}

void ScrollBar::setBackColor(int color)
{
	m_backColor = color;
}

void ScrollBar::setScrollerColor(int color)
{
	m_barColor = color;
}

void ScrollBar::setScrollerPosY(float y)
{
	m_scrollerPosY = y;
}

float ScrollBar::getScrollerPosY()
{
	return m_scrollerPosY;
}