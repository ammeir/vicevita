
/* file_explorer.h: Implementation of a file browser gui tool.

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

#ifndef FILE_EXPLORER_H
#define FILE_EXPLORER_H

#include "navigator.h"
#include "scroll_bar.h"
#include "iRenderable.h"
#include <vector>
#include <string>
#include <string.h> // for strstr

using std::string;
using std::vector;

// Return codes
#define RET_OK 0
#define RET_DIR_OPEN_ERROR		1
#define RET_DIR_READ_ERROR		2
#define RET_DIR_MAKE_ERROR		3
#define RET_DIR_DELETE_ERROR	4
#define RET_FILE_CREATE_ERROR	5
#define RET_FILE_DELETE_ERROR	6
#define RET_FILE_STAT_ERROR		7
#define RET_COPY_DIR_ERROR		8

struct DirEntry
{
	string name;
	string path;
	bool isFile;
};

class vita2d_texture;
class FileExplorer : public Navigator, public IRenderable
{

private:
	string				m_path;
	vector<DirEntry>	m_list;
	char**				m_filter;
	int					m_highlight;
	int					m_borderTop;
	int					m_borderBottom;
	int					m_textColor;
	int					m_highlightColor;
	ScrollBar			m_scrollBar;
	vita2d_texture*		m_folder_icon;
	vita2d_texture*		m_file_icon;
	bool				m_fileSelected;

	void				show();
	void				render();
	void				scrollTo(int hlIndex, int btIndex, float sbPosY, bool showScroll);
	bool				isFileAccepted(const char* fname);
	void				setFilter(const char** filter);
	void				strToUpperCase(string& str);
	void				addParentDirectory();
	
	// Navigator interface implementations
	bool				isExit(int buttons); 
	void				navigateUp(); 
	void				navigateDown(); 
	void				buttonReleased(int button);
	
public:
						FileExplorer();
						~FileExplorer();
	void				init(const char* path, int hlIndex = 0, int btIndex = 0, float sbPosY = 0, const char** filter = NULL);
	string				doModal();
	static FileExplorer* getInst();
	int					readDirContent(const char* path);
	void				sortDirContent();
	vector<DirEntry>&	getDirContent();
	int					makeDir(const char* path);
	int					rmDir(const char* path);
	int					copyDir(const char* src_path, const char* dst_path);
	int					writeToFile(const char* path_to_file, const char* data, long size);
	int					readFromFile(const char* path_to_file, char** data, long* size);
	int					deleteFile(const char* path);
	int					clearDir(const char* path_to_dir);
	void				changeDir(const char* path);
	bool				dirExist(const char* path_to_dir);
	bool				fileExist(const char* path_to_file);
	DirEntry			select();
	string				getFilePath();
	string				getFileName();
	string				getDir();
	string				getDisplayFitString(const char* str, int limit, float font_size = 1.0);
	int					getHighlightIndex();
	int					getBorderTopIndex();
	float				getScrollBarPosY();
	void				setTextColor(int color);
	void				setHighlightColor(int color);
};


#endif