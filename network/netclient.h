#ifndef TOOLCHAIN_NETCLIENT_H
#define TOOLCHAIN_NETCLIENT_H


#include <curl/curl.h>
#include <map>
#include <vector>
#include <atomic>
#include <chrono>


/*
 * @brief Базовый класс, который умеет работать с GET/POST запросами.
 */
class NetClient
{
public:
	NetClient(const std::string &host_);
	virtual ~NetClient();

	CURLcode request_get(
			const std::string &url,
			const std::map<std::string, std::string> &params = {});
	CURLcode send_json(
			const std::string &url,
			const std::map<std::string, std::string> &parameters,
			const std::string &json_data);
	
	CURLcode send_file(FILE *const file, const std::string & filename);
	
	std::string get_data() const;
	int get_last_response_code();
	void stop_curl();
	
private:
	//CURL* curl_handle;
	std::string host;
	std::string received_data;
	const std::chrono::system_clock::duration curl_timeout = std::chrono::minutes(10);
	int last_response_http_code;
	std::atomic<bool> curl_stop_flag;
	std::chrono::system_clock::time_point request_start;
};


std::string map_to_string_parser(const std::map<std::string, std::string> &map_data);
std::string map_to_string_json_parser(const std::map<std::string, std::string> &map_data);
std::map<std::string, std::string> string_to_map_parser(const std::string &string_data);

#endif //TOOLCHAIN_NETCLIENT_H
