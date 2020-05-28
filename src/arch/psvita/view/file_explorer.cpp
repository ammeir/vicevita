
/* file_explorer.cpp: Implementation of a file browser gui tool.

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

#include "file_explorer.h"
#include "scroll_bar.h"
#include "texter.h"
#include "resources.h"
#include "app_defs.h"
#include "debug_psv.h"

#include <algorithm> // std::sort
#include <vita2d.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/io/dirent.h> 
#include <psp2/io/fcntl.h> 


#define MAX_ENTRIES 18 // Max rows on the screen
#define FONT_Y_SPACE 24
#define START_Y 60
#define SCROLL_BAR_X 950
#define SCROLL_BAR_Y 43 
#define SCROLL_BAR_WIDTH 8
#define SCROLL_BAR_HEIGHT (MAX_ENTRIES * FONT_Y_SPACE)
#define SCROLL_BAR_MIN_HEIGHT 4


extern vector<vita2d_texture*> g_instructionBitmaps;
 
static bool compareDirEntries(const DirEntry& entry1, const DirEntry& entry2) 
{
	string str1 = entry1.name;
	string str2 = entry2.name;

	// Can't sort a string if it has a mixture of upper and lower case letters. 
	// For example (B < a) B is incorrectly smaller than a. 
	std::transform(str1.begin(), str1.end(), str1.begin(), ::toupper);
	std::transform(str2.begin(), str2.end(), str2.begin(), ::toupper);
	return str1 < str2;
}


FileExplorer::FileExplorer()
{
	m_file_icon = NULL;
	m_folder_icon = NULL;
	m_filter = NULL;
}

FileExplorer::~FileExplorer()
{
	if (m_file_icon)
		vita2d_free_texture(m_file_icon);
	if (m_folder_icon)
		vita2d_free_texture(m_folder_icon);
	if (m_filter){
		char** p = m_filter;
		while (*p){
			delete[] *p;
			p++;
		}
		delete[] m_filter;
	}
}

void FileExplorer::init(const char* path, int hlIndex, int btIndex, float sbPosY, const char** filter)
{
	m_path = path;
	m_highlight = 0;
	m_borderTop = 0;
	m_borderBottom = MAX_ENTRIES-1;
	m_file_icon = vita2d_load_PNG_buffer(img_file_icon);
	m_folder_icon = vita2d_load_PNG_buffer(img_folder_icon);
	
	setFilter(filter);
	readDirContent(path);
	addParentDirectory();
	sortDirContent();

	m_scrollBar.init(SCROLL_BAR_X, SCROLL_BAR_Y, SCROLL_BAR_WIDTH, SCROLL_BAR_HEIGHT);
	m_scrollBar.setListSize(m_list.size(), MAX_ENTRIES);
	m_scrollBar.setBackColor(GREY);
	m_scrollBar.setBarColor(ROYAL_BLUE);

	// For convenience (for me) we scroll down to last navigation spot. 
	scrollTo(hlIndex, btIndex, sbPosY, false);
}

string FileExplorer::doModal()
{
	m_fileSelected = false;
	show();
	scanCyclic();	

	return m_fileSelected? select().path: "";
}

void FileExplorer::buttonReleased(int button)
{
	if (button == SCE_CTRL_CROSS){ 
		// Select file
		DirEntry entry = select();

		if (entry.isFile){
			m_fileSelected = true;
			Navigator::m_running = false;
			return;
		}
				
		changeDir(entry.path.c_str());
	}
}

bool FileExplorer::isExit(int button)
{
	if (button == SCE_CTRL_LTRIGGER || button == SCE_CTRL_LEFT){ // Previous menu
		return true;
	}

	return false;
}

void FileExplorer::navigateUp()
{
	if (m_highlight > 0){
		if (m_highlight-- == m_borderTop){
			m_borderTop--;
			m_borderBottom--;
			m_scrollBar.scrollUp();
		}

		show();
	}
}

void FileExplorer::navigateDown()
{
	if (m_highlight < (m_list.size()-1)){
		if (m_highlight++ == m_borderBottom){
			m_borderBottom++;
			m_borderTop++;
			m_scrollBar.scrollDown();
		}

		show();
	}
}

void FileExplorer::changeDir(const char* path)
{
	m_path = path;
	m_highlight = 0;
	m_borderTop = 0;
	m_borderBottom = MAX_ENTRIES-1;

	readDirContent(path);
	addParentDirectory();
	sortDirContent();
	m_scrollBar.setListSize(m_list.size(), MAX_ENTRIES);
	show();
}

DirEntry FileExplorer::select()
{
	return m_list[m_highlight];
}

string FileExplorer::getFilePath()
{
	return m_list[m_highlight].path;
}

string FileExplorer::getFileName()
{
	return m_list[m_highlight].name;
}

string FileExplorer::getDir()
{
	return m_path;
}

int	FileExplorer::getHighlightIndex()
{
	return m_highlight;
}

int FileExplorer::getBorderTopIndex()
{
	return m_borderTop;
}

float FileExplorer::getScrollBarPosY()
{
	return m_scrollBar.getScrollerPosY();
}

void FileExplorer::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing();
	vita2d_swap_buffers();
	vita2d_wait_rendering_done(); // Sometimes GPU can crash when loading game. This fixes it.
}

void FileExplorer::render()
{
	int y = START_Y;
	int text_color = YELLOW;
	vita2d_texture* entry_icon;
	
	// Path
	txtr_draw_text(0, 20, C64_BLUE, m_path.c_str());

	// Top seperation line
	vita2d_draw_line(0, 30, 960, 30, YELLOW_TRANSPARENT);

	int start = m_borderTop;
	int end = (m_list.size() > MAX_ENTRIES)? m_borderBottom: m_list.size()-1;

	// Files
	for (int i=start; i<=end; ++i){
		text_color = YELLOW;
		entry_icon = (m_list[i].isFile)? m_file_icon: m_folder_icon;

		vita2d_draw_texture(entry_icon, 0, y-17); 

		if (i == m_highlight){
			text_color = WHITE;
			int textHeight = txtr_get_text_height(m_list[i].name.c_str(), 24);

			// Draw highlight rectangle
			vita2d_draw_rectangle(27, y-textHeight+1, 915, textHeight+2, ROYAL_BLUE);
		}

		txtr_draw_text(30, y, text_color, getDisplayFitString(m_list[i].name.c_str(), 900).c_str());
		y += FONT_Y_SPACE;
	}

	// Scroll bar
	if (m_list.size() > MAX_ENTRIES)
		m_scrollBar.render();

	// Bottom seperation line
	vita2d_draw_line(0, 495, 960, 495, YELLOW_TRANSPARENT);

	// Instructions
	vita2d_draw_texture(g_instructionBitmaps[2], 400, 510); // Navigate buttons
	vita2d_draw_texture(g_instructionBitmaps[3], 490, 510); // Dpad left button
	txtr_draw_text(516, 523 , LIGHT_GREY, "Exit");
}

void FileExplorer::setTextColor(int color)
{

}

void FileExplorer::setHighlightColor(int color)
{

}

int FileExplorer::readDirContent(const char* path)
{
	SceIoDirent dir;

	int fd;

	if ((fd = sceIoDopen(path)) < 0) {
		//PSV_DEBUG("sceIoDopen error: 0x%08X\n, path = %s", fd, path);
	    return RET_DIR_OPEN_ERROR;
	}

	DirEntry entry;
	m_path = path;
	m_list.clear();
	bool entries_left = true;

	// add slash if needed
	if (m_path[m_path.size()-1] != '/') 
		m_path.append("/");

	while (entries_left)
	{
		int ret = sceIoDread(fd, &dir);

		if (ret < 0){
			break;
		}

		if (ret == 0) entries_left = false;

		entry.name = dir.d_name;
		entry.path = m_path + dir.d_name;

		if (dir.d_stat.st_mode & SCE_S_IFREG){ // Is a file?
			if (!isFileAccepted(entry.name.c_str())) continue;
			entry.isFile = true;
		}else{
			entry.isFile = false;
			entry.path += "/";
		}
		
		if (!entry.name.empty()){
			m_list.push_back(entry);
		}
	}

	sceIoDclose(fd);

	return RET_OK;
}

void FileExplorer::addParentDirectory()
{
	// Add ../ dir
	DirEntry entry;

	if (m_path.find_first_of("/") != string::npos){
		entry.name = "..";
		std::string tmp = m_path.substr(0, m_path.size()-1); // Remove last slash
		std::size_t slash_pos = tmp.find_last_of("/:");
		tmp = tmp.substr(0, slash_pos+1);
 
		entry.path = tmp;
		entry.isFile = false;
		m_list.push_back(entry);
	}
}

void FileExplorer::sortDirContent()
{
	std::sort(m_list.begin(), m_list.end(), compareDirEntries);

	vector<DirEntry> tmpList = m_list;

	m_list.clear();

	// Directories first
	for (vector<DirEntry>::iterator it=tmpList.begin(); it!=tmpList.end(); ++it){
		if(!(*it).isFile){
			m_list.push_back(*it);
		}
	}

	// Files
	for (vector<DirEntry>::iterator it=tmpList.begin(); it!=tmpList.end(); ++it){
		if((*it).isFile){
			m_list.push_back(*it);
		}
	}
}

int	FileExplorer::makeDir(const char* path)
{
	if (sceIoMkdir(path, 0777) < 0)
		return RET_DIR_MAKE_ERROR;

	return RET_OK;
}

int	FileExplorer::rmDir(const char* path)
{
	if (sceIoRmdir(path) < 0)
		return RET_DIR_DELETE_ERROR;

	return RET_OK;
}

int	FileExplorer::copyDir(const char* src_path, const char* dst_path)
{
	// Copies only files from source to destination. Skips ones that already exist in the destination folder.
	
	if (!src_path || !dst_path)
		return RET_COPY_DIR_ERROR;

	int ret = readDirContent(src_path);

	if (ret != RET_OK)
		return ret;

	for (int i=0; i<=m_list.size(); ++i){
		
		if (!m_list[i].isFile)
			continue;

		long size;
		char* data = NULL;
		
		ret = readFromFile(src_path, &data, &size);
		
		if (ret != RET_OK){
			if (data) delete[] data;
			return ret;
		}

		ret = writeToFile(dst_path, data, size);
		if (ret != RET_OK){
			delete[] data;
			return ret;
		}

		delete[] data;
	}

	return RET_OK;
}

int	FileExplorer::readFromFile(const char* path_to_file, char** data, long* size)
{
	// Get the status of a file.
	SceIoStat info;
	*data = NULL;
	*size = 0;

	if (sceIoGetstat(path_to_file, &info) < 0)
		return RET_FILE_STAT_ERROR;

	SceUID fd = sceIoOpen(path_to_file, SCE_O_RDONLY|SCE_O_CREAT, 0777);
	
	if (fd < 0){
		return RET_FILE_CREATE_ERROR;
	}

	*data = new char[info.st_size];
	sceIoRead(fd, *data, info.st_size);
	*size = info.st_size;
	sceIoClose(fd);

	return RET_OK;
}

int	FileExplorer::writeToFile(const char* path_to_file, const char* data, long size)
{
	SceUID fd = sceIoOpen(path_to_file, SCE_O_WRONLY|SCE_O_CREAT, 0777);

	if (fd < 0){
		return RET_FILE_CREATE_ERROR;
	}

	if (data){
		int res = sceIoWrite(fd, data, size);
	}

	sceIoClose(fd);

	return RET_OK;
}

vector<DirEntry>& FileExplorer::getDirContent()
{
	return m_list;
}

int	FileExplorer::deleteFile(const char* file)
{
	if (sceIoRemove(file) < 0)
		return RET_FILE_DELETE_ERROR;

	return RET_OK;
}

bool FileExplorer::dirExist(const char* path_to_dir)
{
	SceUID fd = sceIoDopen(path_to_dir); 

	if(fd < 0){
		return false;
	}

	sceIoDclose(fd);

	return true;
}


bool FileExplorer::fileExist(const char* path_to_file)
{
	SceUID fd = sceIoOpen(path_to_file, SCE_O_RDONLY, 0777);
	
	if(fd < 0)
		return false;

	sceIoClose(fd);

	return true;
}

void FileExplorer::scrollTo(int hlIndex, int btIndex, float sbPosY, bool showScroll)
{
	// Scroll to desired last navigation spot. 
	// We need to adjust the highlight rectangle index, border top index and scroll bar y position.


	m_highlight = hlIndex;
	m_borderTop = btIndex;
	m_borderBottom = btIndex + (MAX_ENTRIES-1);
	
	if (sbPosY < SCROLL_BAR_Y)
		m_scrollBar.setScrollerPosY(SCROLL_BAR_Y);
	else
		m_scrollBar.setScrollerPosY(sbPosY);
	
	if (showScroll)
		show();
}

bool FileExplorer::isFileAccepted(const char* fname)
{
	if (!m_filter)
		return true;

	string file = fname;
	string extension;

	size_t dot_pos = file.find_last_of(".");
	if (dot_pos != string::npos)
		extension = file.substr(dot_pos+1, string::npos);

	if (extension.empty())
		return false;

	strToUpperCase(extension);

	char** p = m_filter;

	while (*p){
		if (!strcmp(extension.c_str(), *p)){
			return true;
		}

		p++;
	}

	return false;
}

void FileExplorer::setFilter(const char** filter)
{
	// Filter array must be NULL terminated.

	if (!filter)
		return;

	int size = 0;
	const char** p = filter;
	
	while(*p){
		size++;
		p++;
	}

	m_filter = new char*[size+1]; // One extra for null termination
	for (int i=0; i<size; ++i){
		int alloc_size = strlen(filter[i]+1);
		m_filter[i] = new char[alloc_size];
		memset(m_filter[i], 0, alloc_size);
		strcpy(m_filter[i], filter[i]);
	}

	m_filter[size] = NULL;
}

void FileExplorer::strToUpperCase(string& str)
{
   for (std::string::iterator p = str.begin(); p != str.end(); ++p)
       *p = toupper(*p);
}

string FileExplorer::getDisplayFitString(const char* str, int limit, float font_size)
{
	// Returns a shrinked string that fits to the limit boundaries.

	string ret;

	if (!str || !limit)
		return ret;

	ret = str;
	int str_width = txtr_get_text_width(str, font_size);

	if (str_width <= limit)
		return ret;

	while(str_width > limit){
		ret.pop_back();
		str_width = txtr_get_text_width(ret.c_str(), font_size);
	}

	ret.append("...");

	return ret;
}
