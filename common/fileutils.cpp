#include "fileutils.h"

#include "errutils.h"

#ifdef HAVE_BOOST
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
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

std::list<std::string> ls_directory(const std::string &my_dir,
	const std::set<std::string> &extensions)
{
	std::list<std::string> result;

	using namespace boost::filesystem;
	recursive_directory_iterator it(my_dir);
	for ( ; it != recursive_directory_iterator(); ++it)
	{
		path p = it->path();
		if (!is_regular_file(p))
			continue;
		std::string my_ext = p.extension().string();
		boost::algorithm::to_lower(my_ext);
		if (!extensions.empty() && extensions.find(my_ext) == extensions.end())
			continue;
		std::string full_name = p.generic_string();
//		log_state("FILE '%s'\n", full_name.c_str());
		result.push_back(full_name);
	}

	return result;
}

#else
FILE* fopen_with_backup(const std::string &, const std::string &)
{
	af_assert(!"need compilation with HAVE_BOOST for using 'fopen_with_backup'");
}

std::string get_relative_path(const std::string &, const std::string &)
{
	af_assert(!"need compilation with HAVE_BOOST for using 'get_relative_path'");
}

std::list<std::string> ls_directory(const std::string &,
		const std::set<std::string> &)
{
	af_assert(!"need compilation with HAVE_BOOST for using 'ls_directory'");
}
#endif

std::tuple<std::string, std::string> splitext(const std::string &path)
{
	std::string base = path;
	std::string extension;
	size_t last_slash = base.find_last_of("\\/");
	size_t last_dot = base.find_last_of(".");
	if (last_dot != std::string::npos && last_dot != 0 &&
			last_slash != std::string::npos &&
			last_dot > last_slash)
	{
		base.erase(last_dot);
		extension = path.substr(last_dot);
	}

	return std::make_tuple(base, extension);
}

std::string filename(const std::string &path)
{
	size_t last_slash = path.find_last_of("\\/");
	if (last_slash != std::string::npos)
		return path.substr(last_slash + 1);
	else
		return path;
}

bool file_exists(const std::string &filename)
{
	std::ifstream in_file(filename.c_str());
	return in_file.is_open();
}

} //namespace aifil
