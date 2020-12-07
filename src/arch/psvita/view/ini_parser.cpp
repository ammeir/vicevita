
/* ini_parser.cpp: Simple ini file manager.

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

#include "ini_parser.h"
#include "debug_psv.h"
#include <cstring>


IniParser::IniParser()
{
}

IniParser::~IniParser()
{
}

int IniParser::init(const char* ini_file)
{
	char* buffer = readToBuf(ini_file);

	if (!buffer)
		return INI_PARSER_FILE_NOT_FOUND;

	m_iniFile.loadFromBuf(buffer);

	delete[] buffer;

	return INI_PARSER_OK;
}

int	IniParser::getKeyValue(const char* section, const char* key, const char* ret)
{
	return m_iniFile.getKeyValue(section, key, ret);
}

int	IniParser::setKeyValue(const char* section, const char* key, const char* value)
{
	return m_iniFile.setKeyValue(section, key, value);
}

int	IniParser::addKeyToSec(const char* section, const char* key, const char* value)
{
	return m_iniFile.addKeyToSec(section, key, value);
}

int IniParser::addSection(const char* section)
{
	return m_iniFile.addSection(section);
}

bool IniParser::valuesOccupied(const char* section)
{
	return m_iniFile.valuesOccupied(section);
}

char* IniParser::readToBuf(const char* ini_file)
{
	// Important: This function allocates memory from heap. Caller must deallocate (delete[]) after use.
	FILE *fp = fopen(ini_file, "r");
	
	if (fp == NULL)
		return NULL;

	// Get the file size in bytes
	fseek(fp, 0L, SEEK_END);
	long numbytes = ftell(fp);
 
	char* buffer = new char[numbytes+1]; // One byte more for the terminating null character
	memset(buffer, 0, numbytes+1);

	// Reset the file position indicator to the beginning of the file
	fseek(fp, 0L, SEEK_SET);	
 
	// Copy all the text into the buffer
	fread(buffer, sizeof(char), numbytes, fp);

	fclose(fp);

	return buffer;
}

int	IniParser::saveToFile(const char* ini_file)
{
	return m_iniFile.saveToFile(ini_file);
}

string IniParser::toString()
{
	return m_iniFile.toString();
}

IniFile::IniFile()
{

}

IniFile::~IniFile()
{
}

int	IniFile::loadFromBuf(char* buffer)
{
	char str[128];
	char cr = 0x0D;
	char nl = 0x0A;
	char null = 0x00;
	char delims[3] = {cr,nl,0}; // Delims have to end with a null character
	
	Section sec;
	KeyValuePair kv;

	char* token = strtok(buffer, delims);
	while (token) {

		if(*token == '['){
			// Section
			if (!sec.name.empty()){
				m_sections.push_back(sec);
				sec.name.clear();
				sec.keyValues.clear();
			}

			char* section_start = token + 1;
			char* section_end = strstr(token , "]");
			if(section_end){
				int section_size = section_end - section_start;
				memset(str, 0, 128);
				strncpy(str, section_start, section_size);
				sec.name = str;
			}
		}
		else{
			// Key
			char* key_start = token;
			char* key_end = strstr(token , "=");
			if (key_end){
				int key_size = key_end - key_start;
				memset(str, 0, 128);
				strncpy(str, key_start, key_size);
				kv.key = str;
			}
			// Value
			char* value_start = key_end + 1;
			int value_size = strlen(value_start); // strtok replaces end of the token with a null-character
			memset(str, 0, 128);
			strncpy(str, value_start, value_size);
			kv.value = str;
			sec.keyValues.push_back(kv);
		}
		
		token = strtok(NULL, delims);
	}

	if (!sec.name.empty()){
		m_sections.push_back(sec);
	}

	return 0;
}

int	IniFile::saveToFile(const char* ini_file)
{
	FILE *fp = fopen(ini_file, "w"); 

	if (fp != NULL){
		fprintf(fp, toString().c_str());
		fclose(fp);
		return INI_PARSER_OK;
	}

	return INI_PARSER_FILE_NOT_FOUND;
}

int	IniFile::getKeyValue(const char* section, const char* key, const char* ret)
{
	for(vector<Section>::iterator it=m_sections.begin(); it!=m_sections.end(); ++it){

		if (!(*it).name.compare(section)){
		
			for(vector<KeyValuePair>::iterator it2=(*it).keyValues.begin(); it2!=(*it).keyValues.end(); ++it2){
				if (!(*it2).key.compare(key)){
					strcpy((char*)ret, (*it2).value.c_str());
					return INI_PARSER_OK;
				}
			}
		}
	}

	return INI_PARSER_KEY_NOT_FOUND;
}

int	IniFile::setKeyValue(const char* section, const char* key, const char* value)
{
	for(vector<Section>::iterator it=m_sections.begin(); it!=m_sections.end(); ++it){
		
		if (!(*it).name.compare(section)){
			for(vector<KeyValuePair>::iterator it2=(*it).keyValues.begin(); it2!=(*it).keyValues.end(); ++it2){
				if (!(*it2).key.compare(key)){
					(*it2).value = value;
					return INI_PARSER_OK;
				}
			}

			return INI_PARSER_KEY_NOT_FOUND;
		}
	}

	return INI_PARSER_SECTION_NOT_FOUND;
}

int	IniFile::addKeyToSec(const char* section, const char* key, const char* value)
{
	if (!key)
		return INI_PARSER_ERROR;

	for(vector<Section>::iterator it=m_sections.begin(); it!=m_sections.end(); ++it){
		if (!(*it).name.compare(section)){
			KeyValuePair kv;
			kv.key = key;
			if (value){
				kv.value = value;
			}
			(*it).keyValues.push_back(kv);

			return INI_PARSER_OK;
		}
	}

	return INI_PARSER_SECTION_NOT_FOUND;
}

int IniFile::addSection(const char* section)
{
	if (!section)
		return INI_PARSER_ERROR;

	Section sec;
	sec.name = section;
	m_sections.push_back(sec);

	return INI_PARSER_OK;
}

bool IniFile::valuesOccupied(const char* section)
{
	if (! section)
		return false;

	for(vector<Section>::iterator it=m_sections.begin(); it!=m_sections.end(); ++it){

		if (!(*it).name.compare(section)){
		
			for(vector<KeyValuePair>::iterator it2=(*it).keyValues.begin(); it2!=(*it).keyValues.end(); ++it2){
				if ((*it2).value.empty()){
					return false;
				}
			}
		}
	}

	return true;
}

string IniFile::toString()
{
	string ret;

	char cr_nl[3] = {0x0d, 0x0a, 0x00};
	
	for(vector<Section>::iterator it=m_sections.begin(); it!=m_sections.end(); ++it){

		ret.append("[" + (*it).name + "]\r\n");
		for(vector<KeyValuePair>::iterator it2=(*it).keyValues.begin(); it2!=(*it).keyValues.end(); ++it2){
			ret.append((*it2).key + "=" + (*it2).value + "\r\n");
		}
		
	}

	return ret;
}

