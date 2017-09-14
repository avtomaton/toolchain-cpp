#ifndef STATE_EVENTS_HPP
#define STATE_EVENTS_HPP

#include <cstdint>
#include <string>
#include <mutex>
#include <functional>
#include <memory>
#include <list>


/**
 * @brief
 * Индикатор неинициализированного сообщения.
 */
const uint64_t EMPTY_STATUS_EVENT = 0;


/**
 * @brief
 * Типы сообщений о статусе.
 */
enum class StateEventType {COMMON, STATE, WARNING, ERROR};


/**
 * @brief
 * Сообщение статуса.
 */
struct StateEvent
{
	uint64_t code = EMPTY_STATUS_EVENT;                    ///< Код сообщения.
	std::string human_class;                               ///< Человекочитаемый класс сообщения.
	std::string message;                                   ///< Текстовое описание сообщения.
	StateEventType type = StateEventType::COMMON;          ///< Тип сообщения.

	StateEvent(const uint64_t _code,
	           const std::string & _class,
	           const std::string & _message,
	           const StateEventType _type = StateEventType::ERROR) :
	  code(_code), human_class(_class), message(_message), type(_type)	{}

	StateEvent() {}
};


/**
 * @brief
 * Очередь сообщений.
 */
class StateEventQueue;


/**
 * @brief
 * Нода сети сбора сообщений статуса.
 *
 * Сеть сбора сообщений статуса состоит из произвольного количества нод, связанных
 * между собой. Для присоединения ноды к сети используется вызов attach_to_mesh(),
 * в качестве аргумента которого указывается любая уже присоединенная к сети нода.
 *
 * Построение сети начинается с инициализированной вызовом make() ноды.
 * Последующие ноды присоединяются к любым уже присоединенным к сети нодам.
 *
 * После построения сети все ноды сети равноправны, и могут размещать в сети и
 * читать сообщения. Вызовы размещения, чтения, и удаления сообщений потокобезопасны.
 *
 * Мастер-нода устанавливается вызовом set_master(), и может быть только одна в сети.
 * Мастер-нода отличается от остальных тем, что вызывает функцию обратного вызова,
 * если она установлена.
 * Функция обратного вызова должна максимально быстро завершить работу,
 * так как на время ее выполнения блокируется мьютекс, и размещение в сети
 * новых сообщений блокирует вызывающий код.
 * Функция обратного вызова работает в том потоке, который разместил в сети
 * соответствующее сообщение.
 *
 * При уничтожении мастер-ноды сеть не разрушается, и продолжает функционировать,
 * пока содержит хотя бы одну ноду, но функция обратного вызова прекращает вызываться.
 */
class EventsMeshNode
{
public:
	EventsMeshNode(const EventsMeshNode &) = delete;
	EventsMeshNode(EventsMeshNode &&) = delete;
	EventsMeshNode & operator=(const EventsMeshNode &) = delete;
	EventsMeshNode & operator=(EventsMeshNode &&) = delete;

public:
	EventsMeshNode();
	~EventsMeshNode();

public:
	/**
	 * @brief
	 * Инициализирует ноду. Построение сети начинается с
	 * инициализированной ноды.
	 */
	void make();

	/**
	 * @brief
	 * Переводит ноду в режим мастера. Мастер-нода может быть только одна в сети.
	 * @param handler Функция обратного вызова, которую будет вызывать мастер-нода
	 * при появлении в сети нового сообщения. Функция вызывается только мастер-нодой,
	 * до тех пор, пока существует мастер-нода. Вызов обработчика выполняется в потоке,
	 * который разместил в сети сообщение, и блокирует мьютекс. На время выполнения
	 * обработчика все вызовы, размещающие сообщения, блокируются. В обработчике
	 * нельзя размещать сообщения в этой же сети, так это может привести
	 * к бесконечной блокировке мьютекса.
	 */
	void set_master(const std::function<void(const StateEvent & event)> handler);

	/**
	 * @brief
	 * Присоединяет ноду к сети.
	 * @param Нода, уже присоединенная к сети, или мастер-нода.
	 */
	void attach_to_mesh(const EventsMeshNode & node);

	/**
	 * @brief
	 * Отсоединяет ноду от сети. Если отсоединяется мастер-нода,
	 * она становится обычной неинициализированной нодой.
	 */
	void detach();

	/**
	 * @brief
	 * Размещает в сети сообщение. Нельзя вызывать из обработчика сообщений мастер-ноды.
	 * Потокобезопасен.
	 * @param code Код сообщения.
	 * @param message Текстовое описание сообщения.
	 * @param type Тип сообщения.
	 */
	void push_event(const uint64_t code,
	                const std::string & hclass,
	                const std::string & message,
	                const StateEventType type);

	/**
	 * @brief
	 * Размещает в сети сообщение. Нельзя вызывать из обработчика сообщений мастер-ноды.
	 * Потокобезопасен.
	 * @param event Сообщение.
	 */
	void push_event(const StateEvent & event);

	/**
	 * @brief
	 * Размещает в сети сообщение с типом ERROR.
	 * Нельзя вызывать из обработчика сообщений мастер-ноды.
	 * Потокобезопасен.
	 * @param code Код сообщения.
	 * @param message Текстовое описание сообщения.
	 */
	void push_error(const uint64_t code, const std::string & hclass, const std::string & message);

	/**
	 * @brief
	 * Возвращает копию списка всех сообщений.
	 * @return Список сообщений.
	 */
	std::list<StateEvent> get_events() const;

	/**
	 * @brief
	 * Возвращает копию списка всех сообщений, и удаляет все сообщения из сети.
	 * Потокобезопасен.
	 * @return Список сообщений.
	 */
	std::list<StateEvent> get_events_and_clear();

	/**
	 * @brief
	 * Возвращает первое сообщение из сети.
	 * @return Первое сообщение в сети.
	 */
	StateEvent get_first_event() const;

	/**
	 * @brief
	 * Возвращает последнее сообщение из сети.
	 * @return Последнее сообщение в сети.
	 */
	StateEvent get_last_event() const;

	/**
	 * @brief
	 * Возвращает первое сообщение с типом "ERROR".
	 * @return Первое сообщение в сети.
	 */
	StateEvent get_first_error() const;

	/**
	 * @brief
	 * Удаляет все сообщения из сети.
	 */
	void clear();

private:
	void event_dispatcher(const StateEvent & event);

private:
	std::shared_ptr<StateEventQueue> events_queue;
	std::function<void(const StateEvent & event)> on_event_handler = nullptr;
};

#endif // STATE_EVENTS_HPP
