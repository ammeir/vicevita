/* extractor.cpp: Class for extracting archived files.

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

#include "extractor.h"
#include "file_explorer.h"
#include "app_defs.h"
#include "debug_psv.h"
#include "unzip.h"

#include <cstring>
#include <algorithm> // std::find


Extractor::Extractor()
{
	// Set the destination folders.
	// Different drive folders are used in case we have identical file names in different drives.
	m_trackData[0].dir = TMP_DRV8_DIR;
	m_trackData[1].dir = TMP_DRV9_DIR;
	m_trackData[2].dir = TMP_DRV10_DIR;
	m_trackData[3].dir = TMP_DRV11_DIR;
	m_trackData[4].dir = TMP_DIR;
	m_trackData[5].dir = TMP_DIR;
}

Extractor::~Extractor()
{
}

Extractor* Extractor::getInst()
{
	static Extractor extractor;
	return &extractor;
}

const char* Extractor::extract(const char* archive_file, int drive)
{
	vector<string> tmp_files;
	const char* game_path = archive_file;
	char* file_buffer = NULL;
	unzFile zipfile = NULL;
	
	if (isFileOfType(archive_file, "ZIP")){
		
		char archived_file[512];
		string image_save_path;
		int image_type;
		unz_global_info gi;
		unz_file_info fi;

		// Open Zip for reading
		if (!(zipfile = unzOpen(archive_file))){
			return NULL;
		}

		// Get Zip file information.
		if (unzGetGlobalInfo(zipfile, &gi) != UNZ_OK){
			unzClose(zipfile);
			return NULL;
		}
		
		// Go the first file in the archive.
		if (unzGoToFirstFile(zipfile) != UNZ_OK){
			unzClose(zipfile);
			return NULL;
		}

		for (int i = 0; i < (int)gi.number_entry; i++){
			// Get name of the archived file
			if (unzGetCurrentFileInfo(zipfile, &fi, archived_file, sizeof(archived_file), NULL, 0, NULL, 0) != UNZ_OK){
				goto error;
			}

			image_type = getImageType(archived_file);
			int file_size = fi.uncompressed_size;
		
			if (file_size > 0 && 
				image_type == IMAGE_DISK || 
				image_type == IMAGE_TAPE || 
				image_type == IMAGE_CARTRIDGE || 
				image_type == IMAGE_PROGRAM){

				// Supported file found. 
				
				// Open file for reading
				if(unzOpenCurrentFile(zipfile) != UNZ_OK){
					goto error;
				}

				file_buffer = new char[file_size];
				
				// We read the file in one go.
				// Perhaps this should be read in a loop with smaller buffer size.
				int read = unzReadCurrentFile(zipfile, file_buffer, file_size);
				if (read != file_size){
					goto error;
				}

				// Define the destination image file path. 
				int dev_ind = getDevInd(image_type, drive);
				image_save_path = m_trackData[dev_ind].dir;
				image_save_path.append(archived_file);

				tmp_files.push_back(image_save_path);

				// Create image file from the buffer data.
				FILE* fd = fopen(image_save_path.c_str(), "w");
				if (!fd){
					goto error;
				}
				
				if (fwrite(file_buffer, 1, file_size, fd) < file_size){
					fclose(fd);
					goto error;
				}
				
				fclose(fd);
			
				// Close archived file for reading.
				if(unzCloseCurrentFile(zipfile) != UNZ_OK){
					goto error;
				}

				delete[] file_buffer;
				file_buffer = NULL;
			}
	
			// Go to the next file in the archive
			if (i + 1 < (int)gi.number_entry){
				if (unzGoToNextFile(zipfile) != UNZ_OK){
					goto error;
				}
			}
		}

		unzClose(zipfile);

		if (tmp_files.empty()){
			// No supported files found.
			return NULL;
		}

		image_type = getImageType(tmp_files.front().c_str());
		int dev_ind = getDevInd(image_type, drive);
		
		// Check if we are extracting the same file again.
		if (m_trackData[dev_ind].archive_file != archive_file){
			// Delete and clear all previous tracked files.
			deleteTrackedFiles(m_trackData[dev_ind].files, &tmp_files);
			m_trackData[dev_ind].files.clear();
			// Add new files to track data.
			for (vector<string>::iterator it=tmp_files.begin(); it!=tmp_files.end(); ++it){
				m_trackData[dev_ind].files.push_back((*it));
			}

			m_trackData[dev_ind].archive_file = archive_file;
		}
		
		// Return the first extracted file.
		game_path = m_trackData[dev_ind].files.front().c_str();
	}

	return game_path;

error:
	if (zipfile)
		unzClose(zipfile);

	if (file_buffer)
		delete[] file_buffer;

	// All or nothing approach used here. Function fails on single error.
	// Delete the files that succeeded to extract.
	if (!tmp_files.empty()){
		deleteTrackedFiles(tmp_files);
	}
		
	return NULL;
}

bool Extractor::isFileOfType(const char* fname, const char* type)
{
	string extension;
	string file = fname;

	size_t dot_pos = file.find_last_of(".");
	if (dot_pos != string::npos)
		extension = file.substr(dot_pos+1, string::npos);

	if (extension.empty())
		return false;

	if (extension == type)
		return true;

	strToUpperCase(extension);

	if (extension == type)
		return true;

	return false;
}

int Extractor::getImageType(const char* image)
{
	int ret = -1;
	string extension;
	string file = image;

	static const char* disk_ext[] = {"D64","D71","D80","D81","D82","G64","G41","X64",0};
	static const char* tape_ext[] = {"T64","TAP",0};
	static const char* cart_ext[] = {"CRT",0};
	static const char* prog_ext[] = {"PRG","P00",0};

	size_t dot_pos = file.find_last_of(".");
	if (dot_pos != string::npos)
		extension = file.substr(dot_pos+1, string::npos);

	if (extension.empty())
		return ret;
	
	strToUpperCase(extension);
	
	const char** p = disk_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_DISK;
		p++;
	}
	
	p = tape_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_TAPE;
		p++;
	}
	
	p = cart_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_CARTRIDGE;
		p++;
	}
	
	p = prog_ext;

	while (*p){
		if (!strcmp(extension.c_str(), *p))	
			return IMAGE_PROGRAM;
		p++;
	}

	return ret;
}

int Extractor::getDevInd(int image_type, int drive)
{
	// Return the device index.

	int ret = 0;
	if (image_type == IMAGE_DISK || image_type == IMAGE_PROGRAM){
		ret = drive-8;
	}else if (image_type == IMAGE_TAPE){
		ret = 4;
	}else if (image_type == IMAGE_CARTRIDGE){
		ret = 5;
	}

	return ret;
}

void Extractor::strToUpperCase(string& str)
{
   for (std::string::iterator p = str.begin(); p != str.end(); ++p)
       *p = toupper(*p);
}

void Extractor::deleteTrackedFiles(vector<string>& old_files, vector<string>* new_files)
{
	for (vector<string>::iterator it=old_files.begin(); it!=old_files.end(); ++it){

		if (new_files){
			// Compare to new files. Don't delete a new file of the same name.
			if (std::find(new_files->begin(), new_files->end(), (*it)) != new_files->end())
				continue;
		}

		FileExplorer::getInst()->deleteFile((*it).c_str());
	}
}