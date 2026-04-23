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

//#pragma once // This header is included twice

#include "unique_object.h"
#include "ghost.h"
#include "stack_counter.h"

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

#ifndef UTILS_EVENT_SYNCED
    #define UTILS_EVENT_SYNCED 0
    #define UTILS_EVENT_SYNCED_NOT_PREDEFINED 1
#else
    #define UTILS_EVENT_SYNCED_NOT_PREDEFINED 0
#endif

#if UTILS_EVENT_SYNCED
    #define UTILS_EVENT_CLASS SyncedEvent
    #define UTILS_EVENT_LISTENER_CLASS SyncedEventListener
    #define UTILS_EVENT_VIEW_CLASS SyncedEventView
#else
    #define UTILS_EVENT_CLASS Event
    #define UTILS_EVENT_LISTENER_CLASS EventListener
    #define UTILS_EVENT_VIEW_CLASS EventView
#endif

namespace Utils
{
    template <typename... ParamTypes>
    class UTILS_EVENT_CLASS;

    /// @brief Controls the lifetime of an event listener.
    template <typename... ParamTypes>
    class UTILS_EVENT_LISTENER_CLASS final : private UniqueObject
    {
        friend UTILS_EVENT_CLASS<ParamTypes...>;
    public:
        ~UTILS_EVENT_LISTENER_CLASS();

        UTILS_EVENT_LISTENER_CLASS() = delete;
    private:
        UTILS_EVENT_LISTENER_CLASS(UTILS_EVENT_CLASS<ParamTypes...>* event, std::function<void(ParamTypes...)>);

        GhostOwner<UTILS_EVENT_LISTENER_CLASS> ghost;
#if UTILS_EVENT_SYNCED
        std::shared_ptr<std::mutex> mutex;
#endif

        UTILS_EVENT_CLASS<ParamTypes...>* event;
        std::function<void(ParamTypes...)> listener_function;
    };

#if UTILS_EVENT_SYNCED
    /// @brief A event class providing observer pattern.
    ///        This is not thread-safe.
    template <typename... ParamTypes>
#else
    /// @brief A thread-safe event class providing observer pattern.
    template <typename... ParamTypes>
#endif
    class UTILS_EVENT_CLASS final : private UniqueObject
    {
        friend UTILS_EVENT_LISTENER_CLASS<ParamTypes...>;
    public:
        UTILS_EVENT_CLASS();
        ~UTILS_EVENT_CLASS();

        /// @brief Controls the lifetime of an event listener.
        typedef std::shared_ptr<UTILS_EVENT_LISTENER_CLASS<ParamTypes...>> Listener;

        /// @brief Fires the event by calling all the listeners.
        ///        Not intended to be called recursively.
        void call(ParamTypes...);

        /// @brief Fires the event by calling all the listeners.
        ///        Not intended to be called recursively.
        inline void emit(ParamTypes... params) { call(params...); }
        /// @brief Fires the event by calling all the listeners.
        ///        Not intended to be called recursively.
        inline void invoke(ParamTypes... params) { call(params...); }
        /// @brief Fires the event by calling all the listeners.
        ///        Not intended to be called recursively.
        inline void operator()(ParamTypes... params) { call(params...); }

        /// @brief Adds a listener. The returned listener object MUST be kept to keep listening.
        /// @return A shared pointer of an object to control the lifetime of the listener.
        Listener listen(std::function<void(ParamTypes...)> listener_function);
    private:
#if UTILS_EVENT_SYNCED
        /// Shared between the Event and EventListener objects.
        std::shared_ptr<std::mutex> mutex;
#endif
        StackCounter call_stack_counter;

        std::unordered_set<std::shared_ptr<Ghost<UTILS_EVENT_LISTENER_CLASS<ParamTypes...>>>> listeners;

        /// @brief Used only when a call is happening.
        ///        Will clear and update listeners once the call is ended.
        std::unordered_set<std::shared_ptr<Ghost<UTILS_EVENT_LISTENER_CLASS<ParamTypes...>>>> added_listeners;
        /// @brief Used only when a call is happening.
        ///        Will clear and update listeners once the call is ended.
        std::unordered_set<std::shared_ptr<Ghost<UTILS_EVENT_LISTENER_CLASS<ParamTypes...>>>> removed_listeners;
    };

    /// @brief A restricted view to an Event class. It cannot be fired.
    ///
    /// This can be used as a public member to be listened to externally
    /// while hiding the Event object as a private member.
    /// As a class member, it has to be initialized
    /// in the constructor's member initializer list.
    /// CAUTION: The view must not be available after the event's desctuction.
    template <typename... ParamTypes>
    class UTILS_EVENT_VIEW_CLASS final : private UniqueObject
    {
    public:
        /// CAUTION: The view must not be available after the event's desctuction.
        UTILS_EVENT_VIEW_CLASS(UTILS_EVENT_CLASS<ParamTypes...>& event);
        /// CAUTION: The view must not be available after the event's desctuction.
        UTILS_EVENT_VIEW_CLASS(UTILS_EVENT_CLASS<ParamTypes...>* event);

        /// @brief Adds a listener. The returned listener object MUST be kept to keep listening.
        /// @return A shared pointer of an object to control the lifetime of the listener.
        typename UTILS_EVENT_CLASS<ParamTypes...>::Listener listen(std::function<void(ParamTypes...)> listener_function);
    private:
        UTILS_EVENT_CLASS<ParamTypes...>* event;
    };

    // ---- IMPLEMENTATION ---- //

    template <typename... ParamTypes>
    UTILS_EVENT_LISTENER_CLASS<ParamTypes...>::UTILS_EVENT_LISTENER_CLASS(
            UTILS_EVENT_CLASS<ParamTypes...>* event, std::function<void(ParamTypes...)> func
        ) : ghost(this),
#if UTILS_EVENT_SYNCED
            mutex(event->mutex),
#endif
            event(event), listener_function(func)
    {
        if (event->call_stack_counter.get_count() > 0)
        {
            if (event->removed_listeners.contains(ghost.get_ghost()))
                event->removed_listeners.erase(ghost.get_ghost());
            else
                event->added_listeners.insert(ghost.get_ghost());
        }
        else
        {
            event->listeners.insert(ghost.get_ghost());
        }
    }
    template <typename... ParamTypes>
    UTILS_EVENT_LISTENER_CLASS<ParamTypes...>::~UTILS_EVENT_LISTENER_CLASS()
    {
#if UTILS_EVENT_SYNCED
        std::lock_guard<std::mutex> lock(*mutex.get());
#endif
        if (event != nullptr)
        {
            if (event->call_stack_counter.get_count() > 0)
            {
                if (event->added_listeners.contains(ghost.get_ghost()))
                    event->added_listeners.erase(ghost.get_ghost());
                else
                    event->removed_listeners.insert(ghost.get_ghost());
            }
            else
            {
                event->listeners.erase(ghost.get_ghost());
            }
        }
    }

    template <typename... ParamTypes>
    UTILS_EVENT_CLASS<ParamTypes...>::UTILS_EVENT_CLASS()
#if UTILS_EVENT_SYNCED
        : mutex(std::make_shared<std::mutex>())
#endif
    {
    }
    template <typename... ParamTypes>
    UTILS_EVENT_CLASS<ParamTypes...>::~UTILS_EVENT_CLASS()
    {
#if UTILS_EVENT_SYNCED
        std::lock_guard<std::mutex> lock(*mutex.get());
#endif
        for (auto listener_ghost : listeners)
        {
            if (listener_ghost->get_owner() != nullptr)
                listener_ghost->get_owner()->event = nullptr;
        }
    }

    template <typename... ParamTypes>
    void UTILS_EVENT_CLASS<ParamTypes...>::call(ParamTypes... params)
    {
#if UTILS_EVENT_SYNCED
        auto stack_counter_guard = call_stack_counter.create_guard(mutex.get());
#else
        auto stack_counter_guard = call_stack_counter.create_guard();
#endif
        for (auto listener_ghost : listeners)
        {
#if UTILS_EVENT_SYNCED
            std::function<void(ParamTypes...)> listener_function;
            {
                std::lock_guard<std::mutex> lock(*mutex.get());
                auto listener = listener_ghost->get_owner();
                if (listener == nullptr)
                    continue;
                listener_function = listener->listener_function;
            }
            listener_function(params...);
#else
            auto listener = listener_ghost->get_owner();
            if (listener == nullptr)
                continue;
            listener->listener_function(params...);
#endif
        }

        {
#if UTILS_EVENT_SYNCED
            std::lock_guard<std::mutex> lock(*mutex.get());
#endif
            if (call_stack_counter.get_count() <= 1
                && (!removed_listeners.empty() || !added_listeners.empty()))
            {
                for (auto removed_listener : removed_listeners)
                    listeners.erase(removed_listener);
                for (auto added_listener : added_listeners)
                    listeners.insert(added_listener);
                removed_listeners.clear();
                added_listeners.clear();
            }
        }
    }

    template <typename... ParamTypes>
    std::shared_ptr<UTILS_EVENT_LISTENER_CLASS<ParamTypes...>> UTILS_EVENT_CLASS<ParamTypes...>::listen(
            std::function<void(ParamTypes...)> listener_function)
    {
#if UTILS_EVENT_SYNCED
        std::lock_guard<std::mutex> lock(*mutex.get());
#endif
        std::shared_ptr<UTILS_EVENT_LISTENER_CLASS<ParamTypes...>> result = std::shared_ptr<UTILS_EVENT_LISTENER_CLASS<ParamTypes...>>(
            new UTILS_EVENT_LISTENER_CLASS<ParamTypes...>(this, listener_function)
        );
        return result;
    }

    template <typename... ParamTypes>
    UTILS_EVENT_VIEW_CLASS<ParamTypes...>::UTILS_EVENT_VIEW_CLASS(UTILS_EVENT_CLASS<ParamTypes...>& event) : event(&event)
    {
    }

    template <typename... ParamTypes>
    UTILS_EVENT_VIEW_CLASS<ParamTypes...>::UTILS_EVENT_VIEW_CLASS(UTILS_EVENT_CLASS<ParamTypes...>* event) : event(event)
    {
    }

    template <typename... ParamTypes>
    typename UTILS_EVENT_CLASS<ParamTypes...>::Listener UTILS_EVENT_VIEW_CLASS<ParamTypes...>::listen(
            std::function<void(ParamTypes...)> listener_function)
    {
        return event->listen(listener_function);
    }
}

#undef UTILS_EVENT_CLASS
#undef UTILS_EVENT_LISTENER_CLASS
#undef UTILS_EVENT_VIEW_CLASS

#if UTILS_EVENT_SYNCED_NOT_PREDEFINED
    #undef UTILS_EVENT_SYNCED
#endif

#undef UTILS_EVENT_SYNCED_NOT_PREDEFINED
