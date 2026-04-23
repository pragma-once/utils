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

#include "unique_object.h"
#include "ghost.h"

#include <mutex>
#include <shared_mutex>

namespace Utils
{
    class StackCounter final : private UniqueObject
    {
    public:
        class Guard final : private UniqueObject
        {
            friend StackCounter;
        private:
            Guard(StackCounter* owner) : ghost(owner->ghost.get_ghost())
            {
                owner->count += 1;
            }
        public:
            ~Guard()
            {
                const auto owner = ghost->get_owner();
                if (owner != nullptr)
                    owner->count -= 1;
            }
        private:
            std::shared_ptr<Ghost<StackCounter>> ghost;
        };
        friend Guard;

        template <typename TMutex>
        class GuardWithMutex final : private UniqueObject
        {
            friend StackCounter;
        private:
            std::shared_ptr<Ghost<StackCounter>> ghost;
            TMutex* mutex;

            GuardWithMutex(StackCounter* owner, TMutex* mutex)
                : ghost(owner->ghost.get_ghost()), mutex(mutex)
            {
                std::lock_guard<TMutex> lock(*mutex);
                owner->count += 1;
            }
        public:
            ~GuardWithMutex()
            {
                std::lock_guard<TMutex> lock(*mutex);
                const auto owner = ghost->get_owner();
                if (owner != nullptr)
                    owner->count -= 1;
            }
        };

        StackCounter() : ghost(this), count(0)
        {
        }
    private:
        GhostOwner<StackCounter> ghost;
        int count;
    public:
        constexpr int get_count()
        {
            return count;
        }

        /// @brief Adds 1 to the count while the returned object lives.
        inline Guard create_guard()
        {
            return Guard(this);
        }

        /// @brief Adds 1 to the count while the returned object lives.
        ///        It also locks the mutex when altering the count.
        ///        Use this variant with care.
        /// @param mutex MUST not be nullptr and must not be destructed
        ///              until after the returned guard lives.
        template <typename TMutex>
        inline GuardWithMutex<TMutex> create_guard(TMutex* mutex)
        {
            return GuardWithMutex(this, mutex);
        }
    };
}
