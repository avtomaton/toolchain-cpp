#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <common/errutils.hpp>

#include "netclient.h"


NetClient::NetClient(const std::string &host_) :
	host(host_)
{
	try
	{
		if(curl_global_init(CURL_GLOBAL_ALL) != 0)
			throw std::runtime_error("Function curl_global_init() failed!");

		curl_handle = curl_easy_init();

		if(curl_handle == NULL)
			throw std::runtime_error("Function curl_easy_init() failed!");

		// Отключаем вывод страницы в консоль.
		curl_easy_setopt(curl_handle, CURLOPT_NOBODY,1);
	}
	catch(std::exception e)
	{
		std::cout << e.what() << std::endl;
	}
}


NetClient::~NetClient()
{
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
}


CURLcode NetClient::request_post(const std::map<std::string, std::string> &parameters,
								 const std::string &path) const
{
	std::string post_data = map_to_string_parser(parameters);
	std::string url = host + "/" + path;
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data.c_str());

	return curl_easy_perform(curl_handle);
}


CURLcode NetClient::request_get(const std::string &path) const
{
	std::string url = host + "/" + path;
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
	return curl_easy_perform(curl_handle);
}

CURLcode NetClient::request_get(const std::map<std::string, std::string> &parameters,
								const std::string &path) const
{
	std::string cur_path = path + "?" + map_to_string_parser(parameters);
	return request_get(cur_path);
}


CURLcode NetClient::send_json(const std::map<std::string, std::string> &parameters)
{
	std::string json_data = map_to_string_json_parser(parameters);
	return send_json(json_data);

}


CURLcode NetClient::send_json(const std::string &json_data)
{
	// Настраиваем формат json.
	curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "content-type: application/json");
	headers = curl_slist_append(headers, "charsets: utf-8");

	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl_handle, CURLOPT_URL, host.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, json_data.c_str());

	return curl_easy_perform(curl_handle);
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
