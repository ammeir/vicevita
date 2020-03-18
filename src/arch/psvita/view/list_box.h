
/* list_box.h: Implementation of list box gui element.

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

#ifndef LIST_BOX_H
#define LIST_BOX_H

#include "navigator.h"
#include "scroll_bar.h"
#include <vector>
#include <string>

using std::string;
using std::vector;


class vita2d_texture;
class IRenderable;
class ListBox :public Navigator
{

private:

	int				m_posX;
	int				m_posY;
	int				m_width;
	int				m_height;
	int				m_highlightColor;
	int				m_backgroundColor;
	int				m_frameColor;
	int				m_textColor;
	int				m_frameSize;	
	int				m_indentSize;
	ScrollBar		m_scrollBar;
	ScrollBarSide	m_scrollBarSide;
	int				m_alignment;

	vita2d_texture* m_pal_tex;
	unsigned char*	m_paltex_data;
	unsigned int*	m_palette_data;

	bool			m_visible;
	bool			m_frame;
	bool			m_showArrow;
	bool			m_fullHLBar; // Full highlight bar (same width as the listbox width)
	IRenderable*	m_background;
	
	// Navigator interface implementations
	virtual bool	isExit(int buttons);
	virtual void	navigateUp();
	virtual void	navigateDown();
	virtual void	buttonReleased(int button);
	
	
protected:

	vector<string>	m_list;
	int				m_borderTop;
	int				m_borderBottom;
	bool			m_selected;
	int				m_highlight;
	int				m_textSize;
	int				m_textHeight;
	int				m_maxLinesInView;
	unsigned int	m_exitButton;
	int				m_spacing;


public:
	
					ListBox();
					~ListBox();

	void			init(int x, int y, int width, int height, int frame_size, ScrollBarSide scrollbar_side, int alignment = 1, IRenderable* background = NULL);
	void			render();
	void			show(bool standAlone);
	void			doModal();
	void			addItems(const char* menuEntries[], int size);
	void			clear();
	void			setBackgroundColor(int color);
	void			setFrameColor(int color);
	void			setTextColor(int color);
	void			setSpacing(int val);
	string			getSelected();
	unsigned int	getExitButton();
	void			setVisibility(bool visible);
	bool			isVisible();
	void			showFrame(bool val);
	void			setTextSize(int val);
	void			showArrowOnHighlight(bool val);
	void			setIndentSize(int val);
	void			setFullHighLightBar(bool val);
	void			setHighlighItem(const char* name);
};

#endif