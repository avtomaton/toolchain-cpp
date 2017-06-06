/**
 * @file net-connection.cpp.
 * Реализация функций класса NetConnection.
 */

#include "net-connection.hpp"


NetConnection::NetConnection(uint16_t _listen_port) :
  NetworkConnection(_listen_port),
  read_buffer(READ_BUFFER_SIZE)
{
}


NetConnection::NetConnection(const std::string & _remote_ip,
                             uint16_t _remote_port) :
  NetworkConnection(_remote_ip, _remote_port),
  read_buffer(READ_BUFFER_SIZE)
{
}


NetConnection::~NetConnection()
{
	shutdown();
}


bool NetConnection::on_request_buffer(void * & buffer, size_t & size)
{
	buffer = read_buffer.data();
	size = READ_BUFFER_SIZE;
	return true;
}
