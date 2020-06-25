
/* ini_parser.h: Simple ini file manager.

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

#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <vector>
#include <string>


using std::string;
using std::vector;

enum IniParserRetCodes
{
	INI_PARSER_OK,
	INI_PARSER_ERROR,
	INI_PARSER_SECTION_NOT_FOUND,
	INI_PARSER_KEY_NOT_FOUND,
	INI_PARSER_FILE_NOT_FOUND
};

struct KeyValuePair
{
	string key;
	string value;
};

struct Section
{
	string					name;
	vector<KeyValuePair>	keyValues;
};

class IniFile
{

private:
	string				m_name;
	vector<Section>		m_sections;

public:
						IniFile();
						~IniFile();

	int					loadFromBuf(char* buffer);
	int					loadFromFile(char* ini_file);
	int					saveToFile(const char* ini_file);
	int					getKeyValue(const char* section, const char* key, const char* ret);
	int					setKeyValue(const char* section, const char* key, const char* value);
	int					addKeyToSec(const char* section, const char* key, const char* value);
	string				toString();
					

};

class IniParser
{

private:

	IniFile			m_iniFile;

public:
					IniParser();
					~IniParser();

	int				init(const char* ini_file);
	int				getKeyValue(const char* section, const char* key, const char* ret);
	int				setKeyValue(const char* section, const char* key, const char* value);
	int				addKeyToSec(const char* section, const char* key, const char* value);
	char*			readToBuf(const char* ini_file);
	int				saveToFile(const char* ini_file);

	string			toString();
};

#endif