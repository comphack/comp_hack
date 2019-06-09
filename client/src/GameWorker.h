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
#include <MessageClient.h>
#include <Worker.h>

// libclient Includes
#include <ClientManager.h>

// Standard C++11 Includes
#include <set>

namespace game
{

//
// Forward declaration of managers / game components.
//
class LoginDialog;

/**
 * Worker for client<==>server interaction.
 */
class GameWorker : public QObject, public libcomp::Worker,
    public libcomp::Manager, public std::enable_shared_from_this<GameWorker>
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
     * Add a client manager to process client messages.
     * @param pManager A client message manager
     */
    void AddClientManager(logic::ClientManager *pManager);

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

    /**
     * Get the different types of messages handled by the manager.
     * @return List of message types handled by the manager
     */
    std::list<libcomp::Message::MessageType> GetSupportedTypes() const override;

    /**
     * Process a message from the queue.
     * @param pMessage Message to be processed
     * @return true on success, false on failure
     */
    bool ProcessMessage(const libcomp::Message::Message *pMessage) override;

    /**
     * Wait for a message to enter the queue then handle it
     * with the appropriate @ref Manager configured for the
     * worker.
     * @param pMessageQueue Queue to check for messages
     */
    void Run(libcomp::MessageQueue<
        libcomp::Message::Message*> *pMessageQueue) override;

    /**
     * Get the login dialog.
     * @returns Pointer to the login dialog.
     */
    LoginDialog* GetLoginDialog() const;

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
    /**
     * Process a connection message.
     * @param pMessage Connection message to process.
     */
    // bool ProcessConnectionMessage(
    //     const libcomp::Message::ConnectionMessage *pMessage);

    /**
     * Process a client message.
     * @param pMessage Client message to process.
     */
    bool ProcessClientMessage(
        const libcomp::Message::MessageClient *pMessage);

    /// Message queue for the LogicWorker. Events are sent here.
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mLogicMessageQueue;

    /// List of pointers to client message handlers.
    std::set<logic::ClientManager*> mClientManagers;

    /// Login dialog.
    LoginDialog *mLoginDialog;
};

} // namespace game

#endif // LIBCLIENT_SRC_GAMEWORKER_H
