/* extractor.h: Class for extracting archived files.

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

#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <string>
#include <vector>

using std::string;
using std::vector;

struct ext_data_s
{
	string archive_file;	// Name of the archive file.
	string dir;				// Directory where files are extracted.
	vector<string> files;	// List of the extracted files.
};

class Extractor
{

private:

	// Older extracted files are deleted to prevent overpopulating the directories.
	// This structure is used to track and delete all the previously extracted files for given device.
	ext_data_s			m_trackData[6];

	void				deleteTrackedFiles(vector<string>& old_files, vector<string>* new_files = NULL);
	int					getImageType(const char* image);
	int					getDevInd(int image_type, int drive);
	bool				isFileOfType(const char* fname, const char* type);
	void				strToUpperCase(string& str);
	
public:
						Extractor();
						~Extractor();

	static Extractor*	getInst(); // Get the singleton.
	const char*			extract(const char *path, int drive = 8);
};

#endif