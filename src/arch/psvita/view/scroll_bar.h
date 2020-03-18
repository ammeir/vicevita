
/* scroll_bar.h: Implementation of a window scroll bar gui element.

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

#ifndef SCROLL_BAR_H
#define  SCROLL_BAR_H

enum ScrollBarSide {SCROLL_BAR_LEFT = 0, SCROLL_BAR_RIGHT = 1};

class ScrollBar
{

private:
	
	int				m_posX;
	int				m_posY;
	int				m_width;
	int				m_height;
	int				m_listSize;
	float			m_scrollerPosY;
	int				m_scrollerPosYMin;
	int				m_scrollerPosYMax;
	int				m_scrollerHeight;
	float			m_scrollStep;
	int				m_barColor;
	int				m_backColor;

public:
					ScrollBar();
	virtual			~ScrollBar();
	void			init(int x, int y, int width, int height);
	void			scrollUp();
	void			scrollDown();
	void			render();
	void			setListSize(int size, int max_entries_in_view);
	void			setBarColor(int color);
	void			setBackColor(int color);
	void			setScrollerColor(int color);
	void			setScrollerPosY(float y);
	float			getScrollerPosY();
};


#endif