/**
 * @file libcomp/src/Manager.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base manager class to process messages of a specific type.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_MANAGER_H
#define LIBCOMP_SRC_MANAGER_H

// libcomp Includes
#include "Message.h"

// Standard C++11 Includes
#include <list>

namespace libcomp
{

/**
 * Abstract base class used to represent a @ref Message handler.
 */
class Manager
{
public:
    /**
     * Cleanup the manager.
     */
    virtual ~Manager() { }

    /**
     * Get the different types of messages handled by the manager.
     * @return List of message types handled by the manager
     */
    virtual std::list<Message::MessageType> GetSupportedTypes() const = 0;

    /**
     * Process a message from the queue.
     * @param pMessage Message to be processed
     * @return true on success, false on failure
     */
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage) = 0;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_MANAGER_H
