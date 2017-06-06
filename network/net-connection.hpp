#ifndef NETCONNECTION_HPP
#define NETCONNECTION_HPP

#include "network-connection.hpp"


/**
 * @brief
 *
 * === ! ПРОИЗВОДНЫЙ КЛАСС ОБЯЗАН В ДЕСТРУКТОРЕ ВЫЗВАТЬ МЕТОД shutdown() ! ===
 *
 * Расширение класса NetworkConnection.
 * Предоставляет размещенный в памяти буфер для поступающих через сетевое соединение данных,
 * и передает производному классу указатель на буфер с данными.
 * Предназначен для использования в качестве базового класса для классов с не интенсивным потоком
 * поступающих из сети данных, и допускающих наличие промежуточного буфера для данных.
 * Поступившая из сетевого соединения порция данных помещается в буфер, указатель на который и
 * количество принятых данных передаются производному классу через переопределенный метод
 * on_read_socket(const void *buffer, size_t octets)
 */
class NetConnection : public NetworkConnection
{
public:

	/**
	 * @brief
	 * Конструктор для режима хоста.
	 * @param _listen_port [in] Номер порта для ожидания подключения клиента.
	 */
	explicit NetConnection(uint16_t _listen_port);

	/**
	 * @brief
	 * Конструктор для режима клиента.
	 * @param _remote_ip [in] IP-адрес удаленного хоста.
	 * @param _remote_port [in] Порт удаленного хоста.
	 */
	NetConnection(const std::string & _remote_ip,
				  uint16_t _remote_port);

	/**
	 * @brief
	 * Деструктор класса.
	 */
	virtual ~NetConnection() override;

private:

	/**
	 * @brief
	 * Переопределенный метод. Возвращает указатель на буфер для принятых данных и его размер.
	 */
	virtual bool on_request_buffer(void * & buffer, size_t & size) override;

private:

	/**
	 * @brief
	 * Максимальный размер буффера для чтения одной порции данных из сокета.
	 */
	const size_t READ_BUFFER_SIZE = 8192;
	std::vector<uint8_t> read_buffer;
};

#endif // NETCONNECTION_HPP
