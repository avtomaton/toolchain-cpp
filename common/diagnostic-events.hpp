#ifndef DIAGNOSTIC_EVENTS_HPP
#define DIAGNOSTIC_EVENTS_HPP

#include "state-events.hpp"


/**
 * @file
 * Предопределенные сообщения об ошибках.
 *
 * Код сообщения:
 * Сообщение содержит уникальный код, который соответствует UTC эпохе,
 * выраженной в секундах. Не должен изменяться.
 * При добавлении нового сообщения в список, рекомендуется воспользоваться
 * сайтом https://www.epochconverter.com/, и использовать в качестве
 * значения кода текущее время "The current Unix epoch time is", что обеспечит
 * уникальность кодов.
 *
 * Класс сообщения:
 * Все уникальные сообщения сгруппированы в несколько классов, описываемых
 * уникальными человекочитаемыми строками.
 * Данные строки используются для определения класса сообщения в коде,
 * и не должны изменяться.
 *
 * Текст сообщения:
 * Произвольный текст, описывающий ошибку. Может изменяться.
 */


namespace Diagnostic
{

/**
 * @brief
 * Класс ошибок, которые вызваны ошибками в коде. При появлении ошибки
 * такого класса требуется поиск и исправление ошибки в коде.
 */
const std::string CLASS_ERROR_RUNTIME = "application_error";

/**
 * @brief
 * Класс ошибок установления соединения.
 */
const std::string CLASS_ERROR_CONNECT = "connection_establishment_error";

/**
 * @brief
 * Класс ошибок канала данных, возникающих при обмене данными.
 */
const std::string CLASS_ERROR_COMMUNICATION_CHANNEL = "communication_channel_error";


/**
 * @brief
 * Класс ошибок в логике работы приложения.
 */
const std::string CLASS_ERROR_LOGIC = "application_logic_error";


/**
 * @brief
 * Необработанное исключение в коде потока.
 */
const StateEvent MSG_ERROR_THREAD_UNHANDLED_EXCEPTION
{1501110646, CLASS_ERROR_RUNTIME, "Unhandled exception in the thread."};


/**
 * @brief
 * Непредвиденная ошибка во время работы.
 */
const StateEvent MSG_ERROR_RUNTIME
{1501110667, CLASS_ERROR_RUNTIME, "Runtime error in the device driver."};


/**
 * @brief
 * Ошибка во время установления соединения.
 */
const StateEvent MSG_ERROR_CONNECTING
{1501110680, CLASS_ERROR_CONNECT, "Connection establishment error."};


/**
 * @brief
 * Превышено время ожидания соединения.
 */
const StateEvent MSG_ERROR_CONNECTING_TIMEOUT
{1501110712, CLASS_ERROR_CONNECT, "The connecting timeout."};


/**
 * @brief
 * Ошибка в канале обмена данными.
 */
const StateEvent MSG_ERROR_COMMUNICATION
{1501110689, CLASS_ERROR_COMMUNICATION_CHANNEL, "Communication channel error."};


/**
 * @brief
 * Длительное время по каналу не передаются никакие данные.
 */
const StateEvent MSG_ERROR_COMMUNICATION_TIMEOUT
{1501110703, CLASS_ERROR_COMMUNICATION_CHANNEL, "Timeout of communication inactivity."};


/**
 * @brief
 * Обнаружена ошибка в логике работы.
 */
const StateEvent MSG_ERROR_LOGIC
{1503589133, CLASS_ERROR_LOGIC, "Error in the logic of the application."};

} //namespace

#endif // DIAGNOSTIC_EVENTS_HPP
