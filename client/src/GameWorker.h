/**
 * @file libcomp/src/GameWorker.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Worker for client UI and scene interaction.
 *
 * This file is part of the COMP_hack Test Client (client).
 *
 * Copyright (C) 2012-2019 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCLIENT_SRC_GAMEWORKER_H
#define LIBCLIENT_SRC_GAMEWORKER_H

// Qt Includes
#include <QObject>

// libcomp Includes
#include <Worker.h>

namespace game
{

/**
 * Worker for client<==>server interaction.
 */
class GameWorker : public QObject, public libcomp::Worker
{
    Q_OBJECT

public:
    /**
     * Create a new worker.
     */
    GameWorker(QObject *pParent = nullptr);

    /**
     * Cleanup the worker.
     */
    ~GameWorker() override;

    /**
     * Sent a message to the LogicWorker message queue.
     * @param pMessage Message to send to the LogicWorker.
     * @returns true if the message was sent; false otherwise.
     */
    bool SendToLogic(libcomp::Message::Message *pMessage);

    /**
     * Set the message queue for the LogicWorker. This message queue is used
     * to send events to the logic thread. Get the worker by calling
     * @ref Worker::GetMessageQueue on the LogicWorker.
     * @param messageQueue Reference to the message queue of the LogicWorker.
     */
    void SetLogicQueue(const std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>>& messageQueue);

signals:
    /**
     * Emit a message signal to be caught by this object in the Qt thread.
     * @param pMessage Message to handle.
     */
    void SendMessageSignal(libcomp::Message::Message *pMessage);

private slots:
    /**
     * Catch a message signal by this object in the Qt thread.
     * @param pMessage Message to handle.
     */
    void HandleMessageSignal(libcomp::Message::Message *pMessage);

protected:
    /**
     * Handle an incoming message from the queue.
     * @param pMessage Message to handle from the queue.
     */
    void HandleMessage(libcomp::Message::Message *pMessage) override;

private:
    /// Message queue for the LogicWorker. Events are sent here.
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mLogicMessageQueue;
};

} // namespace game

#endif // LIBCLIENT_SRC_GAMEWORKER_H
