/*
    MIT License

    Copyright (c) 2023 Majidzadeh (hashpragmaonce@gmail.com)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <set>

// NAMESPACE BEGIN
#ifdef PRAGMAONCE_EVENT_NAMESPACE_OVERRIDE
    namespace PRAGMAONCE_EVENT_NAMESPACE_OVERRIDE {
#else
    #ifndef PRAGMAONCE_EVENT_NAMESPACE_REMOVE
        namespace Utilities { // Default
    #endif
#endif

template <typename... ParamTypes>
class Event;

/// @brief Controls the lifetime of an event listener.
template <typename... ParamTypes>
class EventListener
{
    friend Event<ParamTypes...>;
public:
    ~EventListener();

    EventListener() = delete;
    EventListener(EventListener&) = delete;
    EventListener(EventListener&&) = delete;
    EventListener& operator=(EventListener&) = delete;
    EventListener& operator=(EventListener&&) = delete;
private:
    EventListener(Event<ParamTypes...>* event, std::function<void(ParamTypes...)>);

    std::shared_ptr<std::shared_mutex> mutex;

    Event<ParamTypes...>* event;
    std::function<void(ParamTypes...)> listener_function;
};

/// @brief A thread-safe event class providing observer pattern.
template <typename... ParamTypes>
class Event
{
    friend EventListener<ParamTypes...>;
public:
    Event();
    ~Event();

    Event(Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(Event&) = delete;
    Event& operator=(Event&&) = delete;

    /// @brief Controls the lifetime of an event listener.
    typedef std::shared_ptr<EventListener<ParamTypes...>> Listener;

    /// @brief Fires the event by calling all the listeners.
    void call(ParamTypes...);

    /// @brief Fires the event by calling all the listeners.
    constexpr void emit(ParamTypes... params) { call(params...); }
    /// @brief Fires the event by calling all the listeners.
    constexpr void invoke(ParamTypes... params) { call(params...); }
    /// @brief Fires the event by calling all the listeners.
    constexpr void operator()(ParamTypes... params) { call(params...); }

    /// @brief Adds a listener. The returned listener object MUST be kept to keep listening.
    /// @return A shared pointer of an object to control the lifetime of the listener.
    Listener listen(std::function<void(ParamTypes...)> listener_function);
private:
    /// Shared between the Event and EventListener objects.
    std::shared_ptr<std::shared_mutex> mutex;

    std::set<EventListener<ParamTypes...>*> listeners;
};

/// @brief A restricted view to an Event class. It cannot be fired.
///
/// This can be used as a public member to be listened to externally
/// while hiding the Event object as a private member.
/// As a class member, it has to be initialized
/// in the constructor's member initializer list.
/// CAUTION: The view must not be available after the event's desctuction.
template <typename... ParamTypes>
class EventView
{
public:
    /// CAUTION: The view must not be available after the event's desctuction.
    EventView(Event<ParamTypes...>& event);
    /// CAUTION: The view must not be available after the event's desctuction.
    EventView(Event<ParamTypes...>* event);

    EventView(EventView&) = delete;
    EventView(EventView&&) = delete;
    EventView& operator=(EventView&) = delete;
    EventView& operator=(EventView&&) = delete;

    /// @brief Adds a listener. The returned listener object MUST be kept to keep listening.
    /// @return A shared pointer of an object to control the lifetime of the listener.
    typename Event<ParamTypes...>::Listener listen(std::function<void(ParamTypes...)> listener_function);
private:
    Event<ParamTypes...>* event;
};

// ---- IMPLEMENTATION ---- //

template <typename... ParamTypes>
EventListener<ParamTypes...>::EventListener(Event<ParamTypes...>* event, std::function<void(ParamTypes...)> func)
{
    this->event = event;
    mutex = event->mutex;
    listener_function = func;
}
template <typename... ParamTypes>
EventListener<ParamTypes...>::~EventListener()
{
    std::lock_guard<std::shared_mutex> lock(*mutex.get());
    if (event != nullptr)
        event->listeners.erase(this);
}

template <typename... ParamTypes>
Event<ParamTypes...>::Event()
{
    mutex = std::make_shared<std::shared_mutex>();
}
template <typename... ParamTypes>
Event<ParamTypes...>::~Event()
{
    std::lock_guard<std::shared_mutex> lock(*mutex.get());
    for (auto listener : listeners)
        listener->event = nullptr;
}

template <typename... ParamTypes>
void Event<ParamTypes...>::call(ParamTypes... params)
{
    std::shared_lock<std::shared_mutex> lock(*mutex.get());
    for (auto listener : listeners)
        listener->listener_function(params...);
}

template <typename... ParamTypes>
std::shared_ptr<EventListener<ParamTypes...>> Event<ParamTypes...>::listen(std::function<void(ParamTypes...)> listener_function)
{
    std::lock_guard<std::shared_mutex> lock(*mutex.get());
    std::shared_ptr<EventListener<ParamTypes...>> result = std::shared_ptr<EventListener<ParamTypes...>>(
        new EventListener<ParamTypes...>(this, listener_function)
    );
    listeners.insert(result.get());
    return result;
}

template <typename... ParamTypes>
EventView<ParamTypes...>::EventView(Event<ParamTypes...>& event) : event(&event)
{
}

template <typename... ParamTypes>
EventView<ParamTypes...>::EventView(Event<ParamTypes...>* event) : event(event)
{
}

template <typename... ParamTypes>
typename Event<ParamTypes...>::Listener EventView<ParamTypes...>::listen(std::function<void(ParamTypes...)> listener_function)
{
    return event->listen(listener_function);
}

// NAMESPACE END
#ifdef PRAGMAONCE_EVENT_NAMESPACE_OVERRIDE
    }
#else
    #ifndef PRAGMAONCE_EVENT_NAMESPACE_REMOVE // Default
        }
    #endif
#endif
