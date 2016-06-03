#ifndef AIFIL_FILEUTILS_H
#define AIFIL_FILEUTILS_H

#include <fstream>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <cstdio>

namespace aifil {

/**
 * @brief Open new file and backup current one to <file name><postfix>
 * @param fname file name
 * @param postfix postfix for backup (default is '_')
 * @return pointer to FILE object
 */
FILE* fopen_with_backup(const std::string &fname, const std::string &postfix = "_");

/**
 * @brief get path of file relative to parent
 * @param parent folder to split
 * @param file full path
 * @return relative file path
 * @code get_relative_path("/home/user/", "/home/user/aifil/test.cpp")
 * will return "aifil/test.cpp"
 */
std::string get_relative_path(const std::string &parent, const std::string &file);

/**
 * @brief list files in the given directory (recursive)
 * all paths will be relative to given directory
 * @param my_dir directory
 * @param extensions file extensions to add into the list
 * @return list of file paths
 */
std::list<std::string> ls_directory(const std::string &my_dir,
		const std::set<std::string> &extensions = std::set<std::string>());

/**
 * @brief split file name (including path) and extension
 * @param path path to file
 * @return file path without extension and file extension
 */
std::pair<std::string, std::string> splitext(const std::string &path);

/**
 * @brief split path to folder and file name
 * @param path
 * @return folder and file name
 */
std::pair<std::string, std::string> path_split(std::string path);

/**
 * @brief extract file name with extension from full path
 * Output is as from 'basename' utility
 * @param path file path
 * @return file name whithout path
 */
std::string filename(const std::string &path);

bool file_exists(const std::string &filename);

template<typename T>
int read_value_binary(std::istream &stream, T *val, int count = 1)
{
	int bytes = sizeof(T) * count;
	stream.read(reinterpret_cast<char*>(val), bytes);
	return bytes;
}

template<typename T>
int read_value_binary(std::ostream &stream, const T *val, int count = 1)
{
	int bytes = sizeof(T) * count;
	stream.write(reinterpret_cast<const char*>(val), bytes);
	return bytes;
}

//std::string guess_file_path(const std::string& file, bool exception_if_not_found);
//bool path_absolute(const std::string& path);
//void rename_or_move_file(const std::string& from, const std::string& to,
//	bool exception_if_not_found) throw (std::runtime_error);
//FILE* open_or_die(const std::string& filename, const char* mode);
//void write_or_die(FILE* f, const void* data, uint32_t len, const std::string& errgen);
//void read_or_die(FILE* f, void* data, uint32_t len, const std::string& errgen);
//std::string read_file(const std::string& filename);
//void path_is_safe_check(const std::string& path_string); //Throws logic_error if path isn't safe
//std::string safe_filename(const std::string& filename);

} //namespace aifil

#endif //AIFIL_FILEUTILS_H
