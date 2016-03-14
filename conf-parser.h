/** @file conf-parser.h
 *
 *  @brief simple config (ini-like) files parser
 *
 *  @author Victor Pogrebnyak <avtomaton@gmail.com>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef AIFIL_CONF_PARSER_H
#define AIFIL_CONF_PARSER_H

#include "errutils.h"

#include <string>
#include <map>
#include <cstdio>

namespace aifil {

struct ConfigParser
{
	ConfigParser(bool generate_exceptions = true);

	bool exceptions_enabled;

	std::string my_filename;
	std::string my_name;
	std::string my_folder;

	typedef void(* MemberCallback)(ConfigParser*, const std::string&);
	std::map<std::string, std::string*> strings;
	std::map<std::string, int*> ints;
	std::map<std::string, double*> reals;
	std::map<std::string, MemberCallback> callbacks;

	/**
	 * Parses key-value pairs given in string.
	 * Use strings, ints and reals data members to define parse target.
	 * Throws std::runtime_error if something go wrong.
	 */
	void set_file(const std::string &filename);
	void read(const std::string &filename);
	void read(); //re-read my file
	void write(const std::string &filename = "");

	// parse command-line arguments
	void read_args(int argc, char *argv[]);
	std::string usage();

	FILE *file;
	int line_num;
	int skip_lines;
	bool eof;
	std::string line_read();
	static void line_strip(std::string &line);
	void error(const std::string &text, bool force_stop);
	void get_name_and_home(std::string path, std::string &name, std::string &home);

	// standard callbacks
	static void include_parse(ConfigParser *myself, const std::string &val);
};

template<typename T> inline std::string printf_eval() { return std::string(); }
template<> inline std::string printf_eval<int>() { return "%d"; }
template<> inline std::string printf_eval<float>() { return "%f"; }
template<> inline std::string printf_eval<double>() { return "%lf"; }

// handle strings like this:
// <param name 0>0.456 <param name 1>2
// or
// <param name 0>=0.456 <param name 1>=2
template<typename T>
void read_val_from_line(const std::string &line, const std::string &name, T &val)
{
	int name_length = int(name.length());
	size_t pos = line.find(name);
	while (pos != std::string::npos)
	{
		// have space before name
		if (pos != 0 && !isspace(line[pos - 1]))
		{
			pos = line.find(name, pos + 1);
			continue;
		}

		// have data after identifier
		if (pos + name.length() < line.length())
		{
			// have '=' after param name
			if (line[pos + name_length] == '=')
				name_length += 1;

			// have no digit or '-' after param name
			if (!isdigit(line[pos + name_length]) &&
					line[pos + name_length] != '-')
			{
				name_length = int(name.length());
				pos = line.find(name, pos + 1);
				continue;
			}
		}

		break;
	}

	if (pos == std::string::npos || pos + name_length > line.length() - 1)
	{
		log_state("parameter '%s' is not found in '%s'!",
				name.c_str(), line.c_str());
		return;
	}

	except(!isspace(line[pos + name_length]), "wrong config format");
	sscanf(&(line[pos + name_length]), printf_eval<T>().c_str(), &val);
}

// for instantiating for strings
template<>
void read_val_from_line<std::string>(const std::string &line,
	const std::string &name, std::string &val);

} //namespace aifil

#endif // AIFIL_CONF_PARSER_H

