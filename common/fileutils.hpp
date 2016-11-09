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

enum PATH_TYPE {
	PATH_FOLDER = 0x1,
	PATH_FILE = 0x2,
	PATH_SYMLINK = 0x4,
	PATH_OTHER = 0x8,
	PATH_ALL = PATH_FOLDER | PATH_FILE | PATH_SYMLINK | PATH_OTHER
};

#ifdef HAVE_BOOST

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
 * @brief list files in the given directory
 * all paths will be relative to given directory
 * @param my_dir [in] directory
 * @param path_type [in] path types: all, directories, files, symlinks
 * @param recursive [in] do it recursively or not
 * @param extensions [in] file extensions to add into the list
 * @return list of file paths
 * @sa PATH_TYPE
 */
std::list<std::string> ls_directory(
		const std::string &my_dir,
		PATH_TYPE path_types = PATH_ALL,
		bool recursive = true,
		const std::set<std::string> &extensions = std::set<std::string>());

/**
 * @overload std::list<std::string> ls_directory(
 * const std::string &my_dir, const std::set<std::string> &extensions)
 * @details This version looks for regular files only.
 */
std::list<std::string> ls_directory(
		const std::string &my_dir,
		const std::set<std::string> &extensions,
		bool recursive = true);

bool path_exists(const std::string &target_path);

/**
 * @brief Returns parent path if exists or empty string
 * Function does not resolve relative paths to absolute
 * @param str [in] Source path
 * @return Parent path
 */
std::string parent_path(const std::string &str);

/**
 * @return Preferred paths separator in current environment.
 */
char file_separator();

/**
 * @brief Erase file or directory with all content.
 * Acts like @code{.sh}rm -rf path@endcode shell command.
 * @param path [in] Path to file or directory.
 */
void rmtree(const std::string &path);

/**
 * @brief Create full path.
 * Acts like @code{.sh} mkdir -p@endcode shell command.
 * @param path
 * @return true if success, false otherwise
 */
bool makedirs(const std::string &path);

/**
 * @brief Recursive directory structure copying from prototype structure.
 * Files in the source directory are ignored.
 * @param prototype [in] Path with desired directory structure.
 * @param target_path [in] Target path for directory tree creation.
 */
void dir_tree_create(const std::string &prototype, const std::string &target_path);
#endif  // HAVE_BOOST

/**
 * @brief split file name (including path) and extension
 * @param path path to file
 * @code{.cpp}
 * auto file_and_ext = path_split("path/to/my-file.txt");
 * // file_and_ext.first is "path/to/my-file"
 * // file_and_ext.second is ".txt"
 * @endcode
 * @return file path without extension and file extension
 */
std::pair<std::string, std::string> splitext(const std::string &path);

/**
 * @brief split path to folder and file name
 * @param path
 * @code{.cpp}
 * auto file_and_folder = path_split("path/to/my-file.txt");
 * // file_and_folder.first is "path/to"
 * // file_and_folder.second is "my-file.txt"
 * @endcode
 * @return folder and file name
 */
std::pair<std::string, std::string> path_split(const std::string &path);

/**
 * @brief extract file name with extension from the full path
 * Output is as from 'basename' utility
 * @param path file path
 * @code{.cpp}
 * std::string file_name = filename("path/to/my-file.txt");  // returns "my-file.txt"
 * @endcode
 * @return file name whithout path
 */
std::string filename(const std::string &path);

/**
 * @brief Returns file name without extension from the full path
 * @param str [in] Source path
 * @code{.cpp}
 * std::string file_name = stem("path/to/my-file.txt");  // returns "my-file"
 * @endcode
 * @return File name without extension
 */
std::string stem(const std::string &str);

/**
 * @brief Returns file extension from the full path
 * @param str [in] Source path
 * @code{.cpp}
 * std::string file_name = extension("path/to/my-file.txt");  // returns ".txt"
 * @endcode
 * @return File name without extension
 */
std::string extension(const std::string &str);

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

template <class InputIterator>
bool file_write(const std::string &file_name, InputIterator first, InputIterator last)
{
	std::ofstream file(file_name);
	if (!file.is_open())
		return false;

	while (first != last)
	{
		file << *first;
		++first;
	}
	file.close();

	return true;
}

/**
* @brief Amount of lines in file.
* @param file_name [in] Path to file.
* @return Amount of lines (0 if file does not exist).
*/
int line_count_in_file(const std::string &file_name);

} //namespace aifil

#endif //AIFIL_FILEUTILS_H
