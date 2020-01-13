/**
 * @file       session_handler.cpp
 *
 * @author     Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 * @copyright  Apache License Version 2.0
 *
 *      Copyright 2019 Tobias Anker
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "session_handler.h"

#include <network_session/timer_thread.h>
#include <network_session/session_handler.h>

#include <libKitsunemimiProjectNetwork/network_session/session.h>
#include <libKitsunemimiProjectNetwork/network_session/session_controller.h>

#include <libKitsunemimiPersistence/logger/logger.h>
#include <libKitsunemimiNetwork/abstract_socket.h>

namespace Kitsunemimi
{
namespace Project
{

// init static variables
TimerThread* SessionHandler::m_timerThread = nullptr;
SessionHandler* SessionHandler::m_sessionHandler = nullptr;

/**
 * @brief constructor
 */
SessionHandler::SessionHandler(void* sessionTarget,
                               void (*processSession)(void*, bool, Session*, const uint64_t),
                               void* dataTarget,
                               void (*processData)(void*, Session*, const bool,
                                                   const void*, const uint64_t),
                               void* errorTarget,
                               void (*processError)(void*, Session*,
                                                    const uint8_t, const std::string))
{
    m_sessionTarget = sessionTarget;
    m_processSession = processSession;
    m_dataTarget = dataTarget;
    m_processData = processData;
    m_errorTarget = errorTarget;
    m_processError = processError;

    if(m_timerThread == nullptr)
    {
        m_timerThread = new TimerThread();
        m_timerThread->startThread();
    }

    // check if messages have the size of a multiple of 8
    assert(sizeof(Session_Init_Start_Message) % 8 == 0);
    assert(sizeof(Session_Init_Reply_Message) % 8 == 0);
    assert(sizeof(Session_Close_Start_Message) % 8 == 0);
    assert(sizeof(Session_Close_Reply_Message) % 8 == 0);
    assert(sizeof(Heartbeat_Start_Message) % 8 == 0);
    assert(sizeof(Heartbeat_Reply_Message) % 8 == 0);
    assert(sizeof(Error_FalseVersion_Message) % 8 == 0);
    assert(sizeof(Error_UnknownSession_Message) % 8 == 0);
    assert(sizeof(Error_InvalidMessage_Message) % 8 == 0);
    assert(sizeof(Data_SingleStatic_Message) % 8 == 0);
    assert(sizeof(Data_SingleDynamic_Header) % 8 == 0);
    assert(sizeof(Data_SingleReply_Message) % 8 == 0);
    assert(sizeof(Data_MultiInit_Message) % 8 == 0);
    assert(sizeof(Data_MultiInitReply_Message) % 8 == 0);
    assert(sizeof(Data_MultiStatic_Message) % 8 == 0);
    assert(sizeof(Data_MultiFinish_Message) % 8 == 0);
    assert(sizeof(Data_MultiAbort_Message) % 8 == 0);
}

/**
 * @brief destructor
 */
SessionHandler::~SessionHandler()
{
    while (m_sessionMap_lock.test_and_set(std::memory_order_acquire))
                 ; // spin

    // clear maps
    m_sessions.clear();
    m_servers.clear();

    m_sessionMap_lock.clear(std::memory_order_release);

    if(m_timerThread != nullptr)
    {
        delete m_timerThread;
        m_timerThread = nullptr;
    }
}

/**
 * @brief add a new session the the internal list
 *
 * @param id id of the session, which should be added
 * @param session pointer to the session
 */
void
SessionHandler::addSession(const uint32_t id, Session* session)
{
    while (m_sessionMap_lock.test_and_set(std::memory_order_acquire))
                 ; // spin

    session->m_sessionTarget = m_sessionTarget;
    session->m_processSession = m_processSession;
    session->m_dataTarget = m_dataTarget;
    session->m_processData = m_processData;
    session->m_errorTarget = m_errorTarget;
    session->m_processError = m_processError;

    m_sessions.insert(std::pair<uint32_t, Session*>(id, session));

    m_sessionMap_lock.clear(std::memory_order_release);
}

/**
 * @brief remove a session from the internal list, but doesn't close the session
 *
 * @param id id of the session, which should be removed
 */
Session*
SessionHandler::removeSession(const uint32_t id)
{
    Session* ret = nullptr;

    while (m_sessionMap_lock.test_and_set(std::memory_order_acquire))
                 ; // spin

    std::map<uint32_t, Session*>::iterator it;
    it = m_sessions.find(id);

    if(it != m_sessions.end())
    {
        ret = it->second;
        m_sessions.erase(it);
    }

    m_sessionMap_lock.clear(std::memory_order_release);

    return ret;
}

/**
 * @brief increase the internal counter by one and returns the new counter-value
 *
 * @return id for the new session
 */
uint16_t
SessionHandler::increaseSessionIdCounter()
{
    uint16_t tempId = 0;

    while (m_sessionIdCounter_lock.test_and_set(std::memory_order_acquire))
                 ; // spin

    m_sessionIdCounter++;
    tempId = m_sessionIdCounter;

    m_sessionIdCounter_lock.clear(std::memory_order_release);

    return tempId;
}

/**
 * @brief send a heartbeat to all registered sessions
 */
void
SessionHandler::sendHeartBeats()
{
    while (m_sessionMap_lock.test_and_set(std::memory_order_acquire))
                 ; // spin

    std::map<uint32_t, Session*>::iterator it;
    for(it = m_sessions.begin();
        it != m_sessions.end();
        it++)
    {
        it->second->sendHeartbeat();
    }

    m_sessionMap_lock.clear(std::memory_order_release);
}

/**
 * @brief send message over the socket of the session
 *
 * @param session session, where the message should be send
 * @param header reference to the header of the message
 * @param data pointer to the data of the complete data
 * @param size size of the complete data
 */
void
SessionHandler::sendMessage(Session* session,
                            const CommonMessageHeader &header,
                            const void* data,
                            const uint64_t size)
{
    if(header.flags == 0x1)
    {
        SessionHandler::m_timerThread->addMessage(header.type,
                                                  header.sessionId,
                                                  header.messageId,
                                                  session);
    }

    session->m_socket->sendMessage(data, size);
}

} // namespace Project
} // namespace Kitsunemimi
