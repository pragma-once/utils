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

#include <memory>

namespace Utils
{
    template <typename TOwner> class GhostOwner;

    /// @brief An owner can use this to gracefully destruct.
    ///        The owner must have a GhostOwner member to enforce
    ///        automatic handling of construction and destruction.
    ///        The dependencies can have shared_ptr of this each,
    ///        and null-check the result of get_owner on access attempt.
    template <typename TOwner>
    class Ghost final : private UniqueObject
    {
        friend GhostOwner<TOwner>;
    private:
        TOwner* owner;

        Ghost(TOwner* owner)
        {
            this->owner = owner;
        }

        /// @brief Must be called on owner's destruction.
        constexpr void destroy()
        {
            owner = nullptr;
        }
    public:
        /// @brief Returns nullptr if it's destructed.
        constexpr TOwner* get_owner() const
        {
            return owner;
        }
    };

    /// @brief Intended to be used as a member of a ghost owner.
    ///        Dependencies must hold the ghost shared pointer
    ///        returned by get_ghost().
    template <typename TOwner>
    class GhostOwner final : private UniqueObject
    {
    private:
        std::shared_ptr<Ghost<TOwner>> ghost;
    public:
        GhostOwner(TOwner* owner) : ghost(new Ghost<TOwner>(owner))
        {
        }

        ~GhostOwner()
        {
            ghost->destroy();
        }

        constexpr const std::shared_ptr<Ghost<TOwner>>& get_ghost() const
        {
            return ghost;
        }
    };
}
