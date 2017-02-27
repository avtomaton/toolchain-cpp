#include "fileutils.hpp"

#include "errutils.hpp"

#ifdef HAVE_BOOST
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

namespace fs = boost::filesystem;

#endif

namespace aifil {

#ifdef HAVE_BOOST
FILE* fopen_with_backup(const std::string &fname, const std::string &postfix)
{
	using namespace boost::filesystem;
	path old(fname);
	if (exists(old))
		copy_file(fname, fname + postfix, copy_option::overwrite_if_exists);

	FILE *file = 0;
	if (!fname.empty())
		file = fopen(fname.c_str(), "wb");

	return file;
}

std::string get_relative_path(const std::string &parent, const std::string &file)
{
	using namespace boost::filesystem;
	std::string tmp_parent = parent;
	// remove last directory separator
	while (tmp_parent.size() &&
		   (tmp_parent[tmp_parent.size() - 1] == '/' ||
		   tmp_parent[tmp_parent.size() - 1] == '\\'))
		tmp_parent.resize(tmp_parent.size() - 1);

	path file_path(file);
	std::string rest = file_path.filename().string();
	path parent_path(tmp_parent);
	for (int i = 0; i < 20; ++i)
	{
		path cur_parent = file_path.parent_path();
		if (cur_parent == parent_path)
			break;
		rest = cur_parent.filename().string() + '/' + rest;
		std::swap(cur_parent, file_path);
	}
	return rest;
}

// add path to result if it meets type and extension criteria
static void add_proper_path_to_result(
		const fs::path &p, PATH_TYPE path_types,
		const std::set<std::string> &extensions,
		std::list<std::string> &result)
{
	if (fs::is_directory(p) && !(path_types & PATH_FOLDER))
		return;
	if (fs::is_symlink(p) && !(path_types & PATH_SYMLINK))
		return;
	if (fs::is_regular_file(p) && !(path_types & PATH_FILE))
		return;
	if (fs::is_other(p) && !(path_types & PATH_OTHER))
		return;
	if (!extensions.empty())
	{
		std::string my_ext = p.extension().string();
		boost::algorithm::to_lower(my_ext);
		if (extensions.find(my_ext) == extensions.end())
			return;
	}
	std::string full_name = p.generic_string();
	result.push_back(full_name);
}

std::list<std::string> ls_directory(
		const std::string &my_dir,
		PATH_TYPE path_types,
		bool recursive,
		const std::set<std::string> &extensions)
{
	std::list<std::string> result;

	if (!path_exists(my_dir))
	{
		aifil::log_warning("Can't list directory '%s': does not exist", my_dir.c_str());
		return result;
	}

	if (!fs::is_directory(my_dir))
	{
		aifil::log_warning("Can't list '%s': it is not directory", my_dir.c_str());
		return result;
	}

	if (recursive)
	{
		fs::recursive_directory_iterator it(my_dir);
		for ( ; it != fs::recursive_directory_iterator(); ++it)
			add_proper_path_to_result(it->path(), path_types, extensions, result);
	}
	else
	{
		fs::directory_iterator it(my_dir);
		for ( ; it != fs::directory_iterator(); ++it)
			add_proper_path_to_result(it->path(), path_types, extensions, result);
	}

	return result;
}

std::list<std::string> ls_directory(
		const std::string &my_dir,
		const std::set<std::string> &extensions,
		bool recursive)
{
	return ls_directory(my_dir, PATH_FILE, recursive, extensions);
}

bool path_exists(const std::string &target_path)
{
	return fs::exists(fs::path(target_path.c_str()));
}

std::string parent_path(const std::string &str)
{
	return fs::path(str).parent_path().string();
}

char file_separator()
{
	return fs::path::preferred_separator;
}

void rmtree(const std::string &dir_path)
{
	fs::remove_all(fs::path(dir_path));
}

bool makedirs(const std::string &path)
{
	if (path.empty())
		return false;

	fs::path tgt(path);
	if (fs::exists(tgt) && fs::is_directory(tgt))
		return true;  // everything is OK
	else if (fs::exists(tgt))
	{
		aifil::log_warning("makedirs: cannot create '%s', file with the same"
				" path already exists", path.c_str());
		return false;
	}
	else if (!fs::create_directories(tgt))
	{
		aifil::log_warning("makedirs: cannot create '%s'", path.c_str());
		return false;
	}

	return true;
}

void copy(const std::string &from, const std::string &to)
{
	fs::copy(fs::path(from), fs::path(to));
}


void dir_tree_create(const std::string &prototype, const std::string &target_path)
{
	makedirs(target_path);

	std::list<std::string> lst = ls_directory(prototype, aifil::PATH_FOLDER, false);
	for (const auto &node : lst)
	{
		fs::path pnode(node.c_str());
		dir_tree_create((prototype / pnode).string(), (target_path / pnode).string());
	}
}
#endif

std::pair<std::string, std::string> splitext(const std::string &path)
{
	std::string base = path;
	std::string extension;
	size_t last_slash = base.find_last_of("\\/");
	size_t last_dot = base.find_last_of(".");
	if (last_dot != std::string::npos && last_dot != 0 &&
			(last_slash == std::string::npos || last_dot > last_slash))
	{
		base.erase(last_dot);
		extension = path.substr(last_dot);
	}

	return std::pair<std::string, std::string>(base, extension);
}

std::pair<std::string, std::string> path_split(const std::string &path)
{
	std::string home = "./";
	std::string name = path;
	size_t pos = path.find_last_of("\\/");
	if (pos != std::string::npos)
	{
		home = path.substr(0, pos + 1);
		name = path.substr(pos + 1);
	}

	return std::pair<std::string, std::string>(home, name);
}

std::string filename(const std::string &path)
{
#ifdef HAVE_BOOST
	return fs::path(path).filename().string();
#endif
	size_t last_slash = path.find_last_of("\\/");
	if (last_slash != std::string::npos)
		return path.substr(last_slash + 1);
	else
		return path;
}

std::string stem(const std::string &path)
{
#ifdef HAVE_BOOST
	return fs::path(path).stem().string();
#endif
	return splitext(filename(path)).first;
}

std::string extension(const std::string &path)
{
#ifdef HAVE_BOOST
	return fs::path(path).extension().string();
#endif
	return splitext(filename(path)).second;
}

PATH_TYPE path_type(const std::string &path)
{
#ifdef HAVE_BOOST
	if (fs::is_directory(path))
		return PATH_FOLDER;
	else if (fs::is_regular_file(path))
		return PATH_FILE;
	else if (fs::is_symlink(path))
		return PATH_SYMLINK;
	else
		return PATH_OTHER;
#else
	static_assert(!"Not implemented without boost");
#endif
}

bool file_exists(const std::string &filename)
{
	std::ifstream in_file(filename.c_str());
	return in_file.is_open();
}

int line_count_in_file(const std::string &file_name)
{
	std::ifstream f(file_name);
	if (!f.is_open())
		return 0;
	std::string line;
	int num = 0;
	while (std::getline(f, line))
		num++;
	return num;
}


/*
 * TODO: create all tests and move this code into them
TEST(StorageMonitorTest, create_path_for_image_if_needed)
{
	std::string parent_path = "dir2/dir3/img/2016-12-08";
	EXPECT_FALSE(exists(parent_path));

	aifil::makedirs(img_path);(parent_path);
	EXPECT_TRUE(exists(parent_path));

	std::string file_path = parent_path + "/file";
	std::ofstream file(file_path);
	file.close();

	create_path_for_image_if_needed(parent_path);
	EXPECT_TRUE(is_regular_file(file_path));

	remove_all(*path(parent_path).begin());
	EXPECT_FALSE(exists(*path(parent_path).begin()));
}

*/

} //namespace aifil
