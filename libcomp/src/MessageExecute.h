/**
 * @file libcomp/src/MessageExecute.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Indicates that the server should finish initialization.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBCOMP_SRC_MESSAGEEXECUTE_H
#define LIBCOMP_SRC_MESSAGEEXECUTE_H

// libcomp Includes
#include "CString.h"
#include "Message.h"

// Standard C++11 Includes
#include <functional>
#include <memory>
#include <utility>

namespace libcomp
{

namespace Message
{

/**
 * Message that provides code to execute inside the worker.
 */
class Execute : public Message
{
public:
    /**
     * Execute the code contained in the message.
     */
    virtual void Run() = 0;
};

/**
 * Message that provides code to execute inside the worker.
 * @note This is the implementation for a function with a specific signature.
 */
template<typename... Function>
class ExecuteImpl : public Execute
{
public:
    using BindType_t = decltype(std::bind(std::declval<std::function<void(
        Function...)>>(), std::declval<Function>()...));

    /**
     * Create the message.
     */
    template<typename... Args>
    explicit ExecuteImpl(std::function<void(Function...)> f, Args&&... args) :
        Execute(), mBind(std::move(f), std::forward<Args>(args)...)
    {
    }

    /**
     * Cleanup the message.
     */
    virtual ~ExecuteImpl()
    {
    }

    virtual MessageType GetType() const
    {
        return MessageType::MESSAGE_TYPE_SYSTEM;
    }

    virtual void Run()
    {
        mBind();
    }

private:
    BindType_t mBind;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGEEXECUTE_H
