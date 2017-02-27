#ifndef TOOLCHAIN_NETCLIENT_H
#define TOOLCHAIN_NETCLIENT_H


#include <curl/curl.h>
#include <map>


/*
 * @brief Базовый класс, который умеет работать с GET/POST запросами.
 */
class NetClient {
public:
	NetClient(const std::string &host_);
	virtual ~NetClient();
	
private:
	CURL* curl_handle;
	std::string host;

public:
	CURLcode request_get(const std::string &path = "") const;
	CURLcode request_get(const std::map<std::string, std::string> &parameters,
						 const std::string &path = "") const;
	CURLcode request_post(const std::map<std::string, std::string> &parameters,
						  const std::string &path = "") const;
	CURLcode send_json(const std::map<std::string, std::string> &parameters);
	CURLcode send_json(const std::string &json_data);
	
	CURLcode send_file(FILE *const file, const std::string & filename);
	
	
	long get_last_response_code();
};


std::string map_to_string_parser(const std::map<std::string, std::string> &map_data);
std::string map_to_string_json_parser(const std::map<std::string, std::string> &map_data);
std::map<std::string, std::string> string_to_map_parser(const std::string &string_data);

#endif //TOOLCHAIN_NETCLIENT_H
