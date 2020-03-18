
/* menu.h: Main menu window.

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

#ifndef MENU_H
#define MENU_H

#include "navigator.h"
#include "iRenderable.h"
#include <string>

using std::string;

class View;
class vita2d_texture;
class MainMenu :public Navigator
{

private:

	View*			 m_view;
	vita2d_texture*  m_imgMenu;
	vita2d_texture*  m_imgMenuArrow;
	int				 m_highlight;
	bool			 m_selected;
	string			 m_selection;

	int				 getControlPadButtonIndex(unsigned int buttons);

	// Navigator interface implementations
	virtual bool	isExit(int buttons); 
	virtual void	navigateUp();
	virtual void	navigateDown();
	virtual void	buttonReleased(int button);

	void			renderMenu();
	void			loadResources();

public:
					MainMenu();
	virtual			~MainMenu();

	void			init(View* view);
	void			show();
	void			render();
	void			doModal();
	string			getSelection();
};

#endif