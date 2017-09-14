#include "state-events.hpp"


// =================== StateEventQueue =====================================================


/**
 * @brief
 * Ядро сети сбора сообщений.
 * Все ноды, объединенные в сеть, используют одно разделяемое
 * между ними ядро, хранящее в себе все сообщения.
 * Каждая нода служит точкой доступа к общему ядру (и списку сообщений).
 */
class StateEventQueue
{
public:
	/**
	 * @brief
	 * Добавляет в список сообщение.
	 * @param code Уникальный код класса сообщения.
	 * @param message Текст сообщения.
	 * @param type Тип сообщения.
	 */
	void push_event(const uint64_t code,
	                const std::string & hclass,
	                const std::string & message,
	                const StateEventType type);

	/**
	 * @brief
	 * Добавляет в список сообщение.
	 * @param event Сообщение.
	 */
	void push_event(const StateEvent & event);

	/**
	 * @brief
	 * Возвращает полный список сообщений.
	 * @return Список сообщений.
	 */
	std::list<StateEvent> get_events() const;

	/**
	 * @brief
	 * Возвращает список сообщений, и очищает внутренний список.
	 * @return Список сообщений.
	 */
	std::list<StateEvent> get_events_and_clear();

	/**
	 * @brief
	 * Возвращает первое сообщение в списке.
	 * @return Первое сообщение.
	 */
	StateEvent get_first_event() const;

	/**
	 * @brief Возвращает последнее сообщение в списке.
	 * @return Сообщение.
	 */
	StateEvent get_last_event() const;

	/**
	 * @brief Возвращает первое сообщение с типом "ERROR".
	 * @return Сообщение.
	 */
	StateEvent get_first_error() const;

	/**
	 * @brief
	 * Удаляет все сообщения.
	 */
	void clear();

	/**
	 * @brief
	 * Устанавливает функцию обратного вызова, которая будет
	 * вызываться при размещении в очереди нового сообщения.
	 * Если один из параметров равен nullptr, функция удаляется.
	 * @param _on_push_event Функция обратного вызова.
	 * @param _owner Указатель на владельца функции.
	 */
	void set_on_push_event_handler(const std::function<void(const StateEvent &)> _on_push_event,
	                               const void * const _owner);

	/**
	 * @brief
	 * Удаляет функцию обратного вызова по запросу только ее владельца.
	 * @param owner Указатель на владельца функции. Функция обратного вызова
	 * удаляется в том случае, если owner совпадает с owner, переданным при
	 * задании функции.
	 */
	void clear_owned_on_push_event_handler(const void * const owner);

private:
	std::mutex callback_mutex;
	std::function<void(const StateEvent &)> on_push_event = nullptr;
	void * callback_owner = nullptr;

private:
	mutable std::mutex events_mutex;
	std::list<StateEvent> state_events;
};


void StateEventQueue::push_event(const uint64_t code,
                                 const std::string & hclass,
                                 const std::string & message,
                                 const StateEventType type)
{
	StateEvent event;
	event.code = code;
	event.human_class = hclass;
	event.message = message;
	event.type = type;

	push_event(event);
}


void StateEventQueue::push_event(const StateEvent & event)
{
	{
		std::lock_guard<std::mutex> lock(events_mutex);
		state_events.push_back(event);
	}

	{
		std::lock_guard<std::mutex> lock(callback_mutex);

		if (on_push_event)
			on_push_event(event);
	}
}


std::list<StateEvent> StateEventQueue::get_events() const
{
	std::lock_guard<std::mutex> lock(events_mutex);
	return state_events;
}


std::list<StateEvent> StateEventQueue::get_events_and_clear()
{
	std::lock_guard<std::mutex> lock(events_mutex);
	std::list<StateEvent> result;
	std::swap(result, state_events);
	return result;
}


StateEvent StateEventQueue::get_first_event() const
{
	StateEvent event;
	std::lock_guard<std::mutex> lock(events_mutex);

	if (!state_events.empty())
		event = state_events.front();

	return event;
}


StateEvent StateEventQueue::get_last_event() const
{
	StateEvent event;
	std::lock_guard<std::mutex> lock(events_mutex);

	if (!state_events.empty())
		event = state_events.back();

	return event;
}


StateEvent StateEventQueue::get_first_error() const
{
	StateEvent event;
	std::lock_guard<std::mutex> lock(events_mutex);

	for (const StateEvent & e : state_events)
	{
		if (e.type != StateEventType::ERROR)
			continue;

		event = e;
		break;
	}

	return event;
}


void StateEventQueue::clear()
{
	std::lock_guard<std::mutex> lock(events_mutex);
	state_events.clear();
}


void StateEventQueue::set_on_push_event_handler(
    const std::function<void(const StateEvent &)> _on_push_event,
    const void * const _owner)
{
	std::lock_guard<std::mutex> lock(callback_mutex);

	if ((_owner) && (_on_push_event))
	{
		on_push_event = _on_push_event;
		callback_owner = (void *) _owner;
	}
	else
	{
		on_push_event = nullptr;
		callback_owner = nullptr;
	}
}


void StateEventQueue::clear_owned_on_push_event_handler(const void * const owner)
{
	std::lock_guard<std::mutex> lock(callback_mutex);

	if (owner != callback_owner)
		return;

	on_push_event = nullptr;
}



// =================== EventsMeshNode =====================================================


EventsMeshNode::EventsMeshNode()
{
}


EventsMeshNode::~EventsMeshNode()
{
	detach();
}


void EventsMeshNode::make()
{
	events_queue = std::make_shared<StateEventQueue>();
}


void EventsMeshNode::set_master(const std::function<void(const StateEvent &)> handler)
{
	on_event_handler = handler;

	if (events_queue)
	{
		auto dispatcher = std::bind(&EventsMeshNode::event_dispatcher, this, std::placeholders::_1);
		events_queue->set_on_push_event_handler(dispatcher, this);
	}
}


void EventsMeshNode::attach_to_mesh(const EventsMeshNode & node)
{
	events_queue = node.events_queue;
}


void EventsMeshNode::detach()
{
	if (events_queue)
		events_queue->clear_owned_on_push_event_handler(this);

	events_queue.reset();
}


void EventsMeshNode::push_event(const uint64_t code,
                                const std::string & hclass,
                                const std::string & message,
                                const StateEventType type)
{
	if (events_queue)
		events_queue->push_event(code, hclass, message, type);
}


void EventsMeshNode::push_event(const StateEvent & event)
{
	if (events_queue)
		events_queue->push_event(event);
}


void EventsMeshNode::push_error(const uint64_t code,
                                const std::string & hclass,
                                const std::string & message)
{
	if (events_queue)
		events_queue->push_event(code, hclass, message, StateEventType::ERROR);
}


std::list<StateEvent> EventsMeshNode::get_events() const
{
	std::list<StateEvent> result;

	if (events_queue)
		result = events_queue->get_events();

	return result;
}


std::list<StateEvent> EventsMeshNode::get_events_and_clear()
{
	std::list<StateEvent> result;

	if (events_queue)
		result = events_queue->get_events_and_clear();

	return result;
}


StateEvent EventsMeshNode::get_first_event() const
{
	StateEvent event;

	if (events_queue)
		event = events_queue->get_first_event();

	return event;
}


StateEvent EventsMeshNode::get_last_event() const
{
	StateEvent event;

	if (events_queue)
		event = events_queue->get_last_event();

	return event;
}


StateEvent EventsMeshNode::get_first_error() const
{
	StateEvent event;

	if (events_queue)
		event = events_queue->get_first_error();

	return event;
}


void EventsMeshNode::clear()
{
	if (events_queue)
		events_queue->clear();
}


void EventsMeshNode::event_dispatcher(const StateEvent & event)
{
	if (on_event_handler)
		on_event_handler(event);
}
