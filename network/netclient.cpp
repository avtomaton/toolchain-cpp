#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <common/errutils.hpp>
#include <common/stringutils.hpp>

#include "netclient.h"
#include "http-common.hpp"

#include <sys/stat.h>

typedef std::unique_ptr<CURL, void(*)(CURL*)> unique_curl_ptr;

struct CurlTerminatior
{
	CurlTerminatior(
			const std::atomic<bool> & stop_flag_,
			const std::chrono::system_clock::duration timeout_
	)
			: timeout(timeout_), stop_flag(stop_flag_)
	{}
	
	void set_start_to_now()
	{
		start = std::chrono::system_clock::now();
	}
	
	int need_stop()
	{
//		std::cout << "timing: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() << " "
//		          << "flag: " << *stop_flag << std::endl;
		
		bool stop_by_timeout = std::chrono::system_clock::now() - start > timeout;
		bool stop_by_interruption = stop_flag;
		
		if (stop_by_timeout) aifil::log_important("stop_by_timeout");
		if (stop_by_interruption) aifil::log_important("stop_by_interruption");
			
		return stop_by_timeout || stop_by_interruption;
	}

private:
	
	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::duration timeout;
	const std::atomic<bool> & stop_flag;
	
};

int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	CurlTerminatior* t = reinterpret_cast<CurlTerminatior*> (clientp);
	return t->need_stop();
}

static int older_progress_callback(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
{
	return progress_callback(p,
	                         (curl_off_t)dltotal, (curl_off_t)dlnow,
	                         (curl_off_t)ultotal, (curl_off_t)ulnow);
}

void curl_set_stopper(const unique_curl_ptr & curl_handle, CurlTerminatior & stopper)
{
	stopper.set_start_to_now();
	
	curl_easy_setopt(curl_handle.get(), CURLOPT_PROGRESSFUNCTION, older_progress_callback);
	curl_easy_setopt(curl_handle.get(), CURLOPT_PROGRESSDATA, &stopper);

#if LIBCURL_VERSION_NUM >= 0x072000
	curl_easy_setopt(curl_handle.get(), CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl_handle.get(), CURLOPT_XFERINFODATA, &stopper);
#endif
	
	curl_easy_setopt(curl_handle.get(), CURLOPT_NOPROGRESS, 0L);
}

NetClient::NetClient(const std::string &host_) :
	host(host_), curl_stop_flag(false)
{
	if (aifil::endswith(host, "/"))
		host.resize(int(host.size()) - 1);
	try
	{
		if(curl_global_init(CURL_GLOBAL_ALL) != 0)
			throw std::runtime_error("Function curl_global_init() failed!");
	}
	catch(std::exception e)
	{
		std::cout << e.what() << std::endl;
	}
}


NetClient::~NetClient()
{
	curl_global_cleanup();
}

struct url_data {
	char* data;
	size_t size;
	
	url_data() : data(nullptr), size(0)
	{
		data = (char*) malloc(4096); /* reasonable size initial buffer */
		if(data == nullptr)
			af_exception("allocation failed");
		data[0] = '\0';
	}
	
	~url_data()
	{
		if (data)
			free(data);
	}
};

size_t write_callback(
		void *received_data,
		size_t number_of_blocks,
		size_t bytes_in_block,
		struct url_data *data)
{
	size_t currently_written = data->size;
	size_t received_bytes = (number_of_blocks * bytes_in_block);
	
	data->size += (number_of_blocks * bytes_in_block);
	char* tmp = (char*) realloc(data->data, data->size + 1); /* +1 for '\0' */
	
	if (tmp)
		data->data = tmp;
	else
		af_exception("Failed to allocate memory.");
	
	memcpy((data->data + currently_written), received_data, received_bytes);
	data->data[data->size] = '\0';
	return number_of_blocks * bytes_in_block;
}

CURLcode NetClient::request_get(
		const std::string &url,
		const std::map<std::string, std::string> &params)
{
	std::string final_url = host + url;
	if (!params.empty())
		final_url += "?" + map_to_string_parser(params);

	struct url_data data;
	auto curl_handle = std::unique_ptr<CURL, void(*)(CURL*)>(curl_easy_init(), curl_easy_cleanup);
	
	curl_easy_setopt(curl_handle.get(), CURLOPT_URL, final_url.c_str());
	curl_easy_setopt(curl_handle.get(), CURLOPT_HTTPGET, 1);
	curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(curl_handle.get(), CURLOPT_VERBOSE, 0L);
	
	CurlTerminatior stopper(curl_stop_flag, curl_timeout);
	curl_set_stopper(curl_handle, stopper);
	
	CURLcode code = curl_easy_perform(curl_handle.get());
	received_data.clear();
	received_data += data.data;
	
	curl_easy_getinfo(curl_handle.get(), CURLINFO_RESPONSE_CODE, &last_response_http_code);
	return code;
}

std::string NetClient::get_data() const
{
	return received_data;
}

CURLcode NetClient::send_json(
		const std::string &url,
		const std::map<std::string, std::string> &parameters,
		const std::string &json_data)
{
	std::string final_url = host + url;
	if (!parameters.empty())
		final_url += "?" + map_to_string_parser(parameters);

	struct url_data data;
	// Настраиваем формат json.
	curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, (http::CONTENT_TYPE + ": " + http::CONTENT_TYPE_JSON).c_str());
	headers = curl_slist_append(headers, "charsets: utf-8");
	
	auto curl_handle = std::unique_ptr<CURL, void(*)(CURL*)>(curl_easy_init(), curl_easy_cleanup);
	curl_easy_setopt(curl_handle.get(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl_handle.get(), CURLOPT_URL, final_url.c_str());
	curl_easy_setopt(curl_handle.get(), CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle.get(), CURLOPT_POSTFIELDS, json_data.c_str());
	curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(curl_handle.get(), CURLOPT_VERBOSE, 0L);
	
	CurlTerminatior stopper(curl_stop_flag, curl_timeout);
	curl_set_stopper(curl_handle, stopper);
	
	CURLcode ret = curl_easy_perform(curl_handle.get());
	curl_easy_getinfo(curl_handle.get(), CURLINFO_RESPONSE_CODE, &last_response_http_code);

	received_data.clear();
	received_data += data.data;
	
	curl_slist_free_all(headers);
	return ret;
}

CURLcode NetClient::send_file(FILE *const file, const std::string & filename)
{
	curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "charsets: utf-8");
	headers = curl_slist_append(
			headers,
			(http::CONTENT_DISPOSITION + ": filename=\"" + filename + "\"").c_str()
	);
	
	headers = curl_slist_append(headers, (http::CONTENT_TYPE + ": " + http::CONTENT_TYPE_FILE).c_str());
	
	struct stat file_info;
	if(fstat(fileno(file), &file_info) != 0)
		aifil::except("cant figure out size of file");
	
	std::string url = host + "/upload_file?";
	
	auto curl_handle = std::unique_ptr<CURL, void(*)(CURL*)>(curl_easy_init(), curl_easy_cleanup);
	curl_easy_setopt(curl_handle.get(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl_handle.get(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl_handle.get(), CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl_handle.get(), CURLOPT_READDATA, file);
	curl_easy_setopt(curl_handle.get(), CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
	curl_easy_setopt(curl_handle.get(), CURLOPT_VERBOSE, 0L);
	
	CurlTerminatior stopper(curl_stop_flag, curl_timeout);
	curl_set_stopper(curl_handle, stopper);
	
	CURLcode ret = curl_easy_perform(curl_handle.get());
	curl_easy_getinfo(curl_handle.get(), CURLINFO_RESPONSE_CODE, &last_response_http_code);
	
	curl_slist_free_all(headers);
	return ret;
}

int  NetClient::get_last_response_code()
{
	return last_response_http_code;
}

std::string map_to_string_parser(const std::map<std::string, std::string> &map_data)
{
	std::string ret;

	if (map_data.empty())
		return ret;

	for (auto map_element : map_data)
		ret += map_element.first + "=" + map_element.second + "&";

	// Возвращаем сроку без последнего "&".
	if (!ret.empty())
		ret.resize(int(ret.size()) - 1);
	return ret;
}

std::string map_to_string_json_parser(const std::map<std::string, std::string> &map_data)
{
	std::string result_json_string = "{";
	for (auto map_element : map_data)
		result_json_string += "\"" + map_element.first + "\":\"" + map_element.second + "\",";

	// Возвращаем сроку без последней ",".
	size_t string_size = result_json_string.size();
	return std::string(result_json_string.c_str(), result_json_string.c_str()+string_size-1) + "}";
}

std::map<std::string, std::string> string_to_map_parser(const std::string &string_data)
{
	std::map<std::string, std::string> result_map;

	if (string_data.empty())
		return result_map;

	// Делим строку по & и получается <ключ=значение>
	std::vector<std::string> strings;
	boost::split(strings, string_data, boost::is_any_of("&"));
	// Делим строки по = и распихиаваем в map.
	for (std::string pair_key_value : strings)
	{
		std::vector<std::string> map_element;
		boost::split(map_element, pair_key_value, boost::is_any_of("="));
		result_map[map_element[0]] = map_element[1];
	}

	return result_map;
}

void NetClient::stop_curl()
{
	curl_stop_flag = true;
}
