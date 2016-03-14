/** @file conf-parser.cpp
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

#include "conf-parser.h"

#include "errutils.h"
#include "stringutils.h"

#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace aifil {

static std::string default_extension = ".conf";

template<>
void read_val_from_line<std::string>(const std::string &line,
	const std::string &name, std::string &val)
{
	size_t pos = line.find_first_of(name);
	char buf[2048];
	if (pos != std::string::npos)
		sscanf(&(line[pos + name.length()]), "\"%2047[-0-9a-zA-Z ]s\"", buf);
	buf[2047] = '\0';
	val = buf;
}

ConfigParser::ConfigParser(bool generate_exceptions)
	: exceptions_enabled(generate_exceptions),
	  file(0), line_num(0), skip_lines(0), eof(true)
{
	callbacks["!include"] = include_parse;
}

void ConfigParser::get_name_and_home(std::string path, std::string &name, std::string &home)
{
	size_t pos = 0;
	while ((pos = path.find_first_of("\\")) != std::string::npos)
		path.replace(pos, 1, 1, '/');

	pos = path.find_last_of("\\/");
	if (pos > 0)
		home = path.substr(0, pos + 1);
	else
		home = "./";

	name = path.substr(pos + 1, path.find_last_of('.') - pos - 1);
}

void ConfigParser::include_parse(ConfigParser *self, const std::string &val)
{
	int tmp_line_num = self->line_num;
	std::string tmp_fname = self->my_filename;
	if (self->file)
		fclose(self->file);
	self->file = 0;
	self->my_filename = self->my_folder + val;
	self->skip_lines = 0;

	try {
		self->read();
	} catch (const std::runtime_error &) {
		self->error(stdprintf("error reading file '%s', line %d", val.c_str(), tmp_line_num), false);
	}

	if (self->file)
		fclose(self->file);
	self->file = 0;
	self->my_filename = tmp_fname;
	self->skip_lines = tmp_line_num;
	self->read();
}

std::string ConfigParser::line_read()
{
	std::string line_str;
	while (file && !eof)
	{
		char byte;
		int r = (int)fread(&byte, 1, 1, file);
		if (r == 0)
		{
			eof = true;
			break;
		}

		if (r != 1)
		{
			error(stdprintf("cannot read line %d", line_num), true);
			break;
		}

		if (byte == '\r')
			continue;
		if (byte == '\n')
		{
			++line_num;
			break;
		}

		line_str += byte;
	}
	return line_str;
}

void ConfigParser::line_strip(std::string &line)
{
	bool text_started = false;
	int trunc_from = -1;
	for (size_t i = 0; i < line.length(); )
	{
		bool delim = line[i] == ' ' || line[i] == '\t';
		bool comment = line[i] == '#';

		if (!text_started && delim)
		{
			if (line.length() != 1)
				line = line.substr(1, line.length() - 1);
			else
				line.clear();
			continue;
		}

		if (comment)
		{
			if (trunc_from == -1)
				trunc_from = i;
			break;
		}

		if (!delim)
		{
			text_started = true;
			trunc_from = -1;
		}

		if (text_started && delim)
			trunc_from = i;

		++i;
	}
	if (trunc_from == 0)
		line.clear();
	else if (trunc_from != -1)
		line = line.substr(0, trunc_from);
}

void ConfigParser::set_file(const std::string &filename)
{
	my_filename = filename;
	get_name_and_home(filename, my_name, my_folder);
}

void ConfigParser::read(const std::string &filename)
{
	set_file(filename);
	read();
}

void ConfigParser::read()
{
	file = fopen(my_filename.c_str(), "rb");
	if (!file)
	{
		error("cannot read file " + my_filename, true);
		return;
	}

	line_num = 1;
	eof = false;

	while (file && !eof)
	{
		std::string line = line_read();
		line_strip(line);
		if (line.empty() || line_num <= skip_lines)
			continue;

		char key[1024];
		int r = sscanf(line.c_str(), "%s", key);
		if (r == 1)
		{
			char* val = &(line[strlen(key)]);
			while (*val && (*val==' ' || *val=='\t'))
				val++;
			if (strings.find(key) != strings.end())
				*strings[key] = val;
			else if (ints.find(key) != ints.end())
				*ints[key] = atoi(val);
			else if (reals.find(key) != reals.end())
				*reals[key] = atof(val);
			else if (callbacks.find(key) != callbacks.end())
				callbacks[key](this, val);
			else
				error(stdprintf("unknown parameter '%s', line %d", key, line_num), false);
		}
		else
			error(stdprintf("cannot interpret line %d", line_num), false);
	} //for each line

	if (file)
		fclose(file);
	file = 0;
}

void ConfigParser::write(const std::string &filename)
{
	if (filename.empty())
		file = fopen(my_filename.c_str(), "wb");
	else
		file = fopen(filename.c_str(), "wb");

	if (!file)
	{
		error("cannot write file " + filename, true);
		return;
	}

	for (std::map<std::string, std::string*>::const_iterator it0 = strings.begin();
		 it0 != strings.end(); ++it0)
		fprintf(file, "%s %s\n", it0->first.c_str(), it0->second->c_str());
	for (std::map<std::string, int*>::const_iterator it1 = ints.begin();
		 it1 != ints.end(); ++it1)
		fprintf(file, "%s %d\n", it1->first.c_str(), *(it1->second));
	for (std::map<std::string, double*>::const_iterator it2 = reals.begin();
		 it2 != reals.end(); ++it2)
		fprintf(file, "%s %lf\n", it2->first.c_str(), *(it2->second));

	fclose(file);
	file = 0;
	eof = true;
}

void ConfigParser::read_args(int argc, char *argv[])
{
	get_name_and_home(argv[0], my_name, my_folder);
	bool have_error = false;
	for (int a = 1; a < argc; ++a)
	{
		if (strlen(argv[a]) < 3)
		{
			error(stdprintf("cannot parse parameter '%s'", argv[a]), false);
			have_error = true;
			continue;
		}

		std::string param_name(&(argv[a][2]));
		if (param_name == "help")
		{
			printf("%s", usage().c_str());
			exit(0);
		}
		if (strings.find(param_name) != strings.end() && a + 1 < argc)
		{
			*strings[param_name] = argv[a + 1];
			a += 1;
		}
		else if (ints.find(param_name) != ints.end() && a + 1 < argc)
		{
			*ints[param_name] = atoi(argv[a + 1]);
			a += 1;
		}
		else if (reals.find(param_name) != reals.end() && a + 1 < argc)
		{
			*reals[param_name] = atof(argv[a + 1]);
			a += 1;
		}
		else
		{
			error(stdprintf("cannot parse parameter '%s'", argv[a]), false);
			have_error = true;
		}
	}

	if (have_error)
		printf("%s", usage().c_str());
}

std::string ConfigParser::usage()
{
	//TODO: help string
	std::string res("Usage: ");
	res += my_name;
	for (std::map<std::string, std::string*>::const_iterator it0 = strings.begin();
		 it0 != strings.end(); ++it0)
		res += " [--" + it0->first + " VALUE]";
	for (std::map<std::string, int*>::const_iterator it1 = ints.begin();
		 it1 != ints.end(); ++it1)
		res += " [--" + it1->first + " VALUE]";
	for (std::map<std::string, double*>::const_iterator it2 = reals.begin();
		 it2 != reals.end(); ++it2)
		res += " [--" + it2->first + " VALUE]";
	res += "\n";
	return res;
}

void ConfigParser::error(const std::string &text, bool force_stop)
{
	if (exceptions_enabled || force_stop)
	{
		eof = true;
		if (file)
			fclose(file);
		file = 0;
	}
	std::string full_text = "ConfigParser error: " + text;
	if (exceptions_enabled || force_stop)
		throw std::runtime_error(full_text);
	else
		printf("%s\n", full_text.c_str());
}

} //namespace aifil
