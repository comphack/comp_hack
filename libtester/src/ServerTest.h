/**
 * @file libtester/src/ServerTest.h
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Function to assist with testing a suite of server applications.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBTESTER_SRC_SERVERTEST_H
#define LIBTESTER_SRC_SERVERTEST_H

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libcomp Includes
#include <DayCare.h>

// Standard C++11 Includes
#include <chrono>
#include <string>
#include <thread>
#include <functional>
#include <future>

namespace libtester
{

class TestFailure { };

} // namespace libtester

#define UPHOLD_EQ(a, b) { bool _didUpholdCondition = false; \
    ([&](bool& _didUpholdConditionRef) -> void { \
    ASSERT_EQ(a, b); \
    _didUpholdConditionRef = true; })(_didUpholdCondition); \
    if(!_didUpholdCondition) { throw libtester::TestFailure(); } }

#define UPHOLD_GT(a, b) { bool _didUpholdCondition = false; \
    ([&](bool& _didUpholdConditionRef) -> void { \
    ASSERT_GT(a, b); \
    _didUpholdConditionRef = true; })(_didUpholdCondition); \
    if(!_didUpholdCondition) { throw libtester::TestFailure(); } }

#define UPHOLD_TRUE(a) { bool _didUpholdCondition = false; \
    ([&](bool& _didUpholdConditionRef) -> void { \
    ASSERT_TRUE(a); \
    _didUpholdConditionRef = true; })(_didUpholdCondition); \
    if(!_didUpholdCondition) { throw libtester::TestFailure(); } }

#define UPHOLD_FALSE(a) { bool _didUpholdCondition = false; \
    ([&](bool& _didUpholdConditionRef) -> void { \
    ASSERT_FALSE(a); \
    _didUpholdConditionRef = true; })(_didUpholdCondition); \
    if(!_didUpholdCondition) { throw libtester::TestFailure(); } }

#define ASSERT_TRUE_OR_RETURN(cond) { \
    bool res = false; \
    [&]() { \
        ASSERT_TRUE(cond); \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_TRUE_OR_RETURN_MSG(cond, msg) { \
    bool res = false; \
    [&]() { \
        ASSERT_TRUE(cond) << msg; \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_FALSE_OR_RETURN(cond) { \
    bool res = false; \
    [&]() { \
        ASSERT_FALSE(cond); \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_FALSE_OR_RETURN_MSG(cond, msg) { \
    bool res = false; \
    [&]() { \
        ASSERT_FALSE(cond) << msg; \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_EQ_OR_RETURN(a, b) { \
    bool res = false; \
    [&]() { \
        ASSERT_EQ(a, b); \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_EQ_OR_RETURN_MSG(a, b, msg) { \
    bool res = false; \
    [&]() { \
        ASSERT_EQ(a, b) << msg; \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_NE_OR_RETURN(a, b) { \
    bool res = false; \
    [&]() { \
        ASSERT_NE(a, b); \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_NE_OR_RETURN_MSG(a, b, msg) { \
    bool res = false; \
    [&]() { \
        ASSERT_NE(a, b) << msg; \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_GT_OR_RETURN(a, b) { \
    bool res = false; \
    [&]() { \
        ASSERT_GT(a, b); \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_GT_OR_RETURN_MSG(a, b, msg) { \
    bool res = false; \
    [&]() { \
        ASSERT_GT(a, b) << msg; \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_GE_OR_RETURN(a, b) { \
    bool res = false; \
    [&]() { \
        ASSERT_GE(a, b); \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

#define ASSERT_GE_OR_RETURN_MSG(a, b, msg) { \
    bool res = false; \
    [&]() { \
        ASSERT_GE(a, b) << msg; \
        res = true; \
    }(); \
\
    if(!res) { \
        return false; \
    } \
}

namespace libtester
{

class ServerTestConfig
{
public:
    ServerTestConfig(const std::chrono::milliseconds& testTime,
        const std::chrono::milliseconds& bootTime,
        const std::string& programsPath,
        bool debug = false);

    std::chrono::milliseconds GetTestTime() const;
    std::chrono::milliseconds GetBootTime() const;
    std::string GetProgramsPath() const;
    bool GetDebug() const;

private:
    std::chrono::milliseconds mTestTime;
    std::chrono::milliseconds mBootTime;
    std::string mProgramsPath;
    bool mDebug;
};

class TimedTask
{
public:
    virtual void Run() = 0;
};

template<typename... Function>
class TimedTaskImpl : public TimedTask
{
public:
    using BindType_t = decltype(std::bind(std::declval<std::function<void(
        Function...)>>(), std::declval<Function>()...));

    template<typename... Args>
    TimedTaskImpl(std::function<void(Function...)> f, Args&&... args) :
        TimedTask(), mBind(std::move(f), std::forward<Args>(args)...)
    {
    }

    virtual ~TimedTaskImpl()
    {
    }

    virtual void Run()
    {
        mBind();
    }

private:
    BindType_t mBind;
};

} // namespace libtester

template<class Rep, class Period, typename Function, typename... Args>
static inline void EXPECT_COMPLETE(
    const std::chrono::duration<Rep, Period>& dur,
    Function&& f, Args&&... args)
{
    std::promise<bool> promisedFinished;
    auto futureResult = promisedFinished.get_future();

    std::shared_ptr<libtester::TimedTask> task(
        new libtester::TimedTaskImpl<Args...>(
        std::forward<Function>(f), std::forward<Args>(args)...));

    std::thread([](std::promise<bool>& finished,
        const std::shared_ptr<libtester::TimedTask>& _task)
    {
        try
        {
            _task->Run();
        }
        catch(libtester::TestFailure&)
        {
        }

        finished.set_value(true);
    }, std::ref(promisedFinished), task).detach();

    EXPECT_TRUE(futureResult.wait_for(dur) !=
        std::future_status::timeout);
}

template<class Rep, class Period, typename Function, typename... Args>
static inline void EXPECT_TIMEOUT(
    const std::chrono::duration<Rep, Period>& dur,
    Function&& f, Args&&... args)
{
    std::promise<bool> promisedFinished;
    auto futureResult = promisedFinished.get_future();

    std::shared_ptr<libtester::TimedTask> task(
        new libtester::TimedTaskImpl<Args...>(
        std::forward<Function>(f), std::forward<Args>(args)...));

    std::thread([](std::promise<bool>& finished,
        const std::shared_ptr<libtester::TimedTask>& _task)
    {
        try
        {
            _task->Run();
        }
        catch(libtester::TestFailure&)
        {
        }

        finished.set_value(true);
    }, std::ref(promisedFinished), task).detach();

    EXPECT_FALSE(futureResult.wait_for(dur) !=
        std::future_status::timeout);
}

template<typename Function, typename... Args>
static inline void EXPECT_SERVER(const libtester::ServerTestConfig& config,
    Function&& f, Args&&... args)
{
    std::promise<bool> promisedFinished;
    auto futureResult = promisedFinished.get_future();

    std::shared_ptr<libtester::TimedTask> task(
        new libtester::TimedTaskImpl<Args...>(
        std::forward<Function>(f), std::forward<Args>(args)...));

    std::thread([](std::promise<bool>& finished,
        const std::chrono::milliseconds& _bootDur,
        const std::string& _programsPath,
        const std::shared_ptr<libtester::TimedTask>& _task,
        bool _debug)
    {
        std::promise<bool> promisedStart;
        auto future = promisedStart.get_future();

        libcomp::DayCare procManager(_debug, [&](){
            promisedStart.set_value(true);
        });

        ASSERT_TRUE(procManager.DetainMonsters(_programsPath));

        auto futureStatus = future.wait_for(_bootDur);

        EXPECT_TRUE(std::future_status::timeout != futureStatus)
            << "Server(s) did not start.";

        if(std::future_status::timeout != futureStatus)
        {
            try
            {
                _task->Run();
            }
            catch(libtester::TestFailure&)
            {
            }
        }

        procManager.CloseDoors();
        procManager.WaitForExit();

        finished.set_value(true);
    }, std::ref(promisedFinished), config.GetBootTime(),
        config.GetProgramsPath(), task, config.GetDebug()).detach();

    EXPECT_TRUE(futureResult.wait_for(config.GetTestTime()) !=
        std::future_status::timeout);
}

namespace libtester
{

namespace ServerConfig
{

static inline ServerTestConfig LobbyOnly()
{
    return ServerTestConfig(
        std::chrono::milliseconds(60000), // 60 seconds
        std::chrono::milliseconds(20000), // 20 seconds
        "bin/testing/programs-lobby.xml",
        false /* debug */);
}

static inline ServerTestConfig SingleChannel()
{
    return ServerTestConfig(
        std::chrono::milliseconds(3 * 60000), // 3 minutes
        std::chrono::milliseconds(60000), // 60 seconds
        "bin/testing/programs.xml",
        false /* debug */);
}

} // namespace ServerConfig

} // namespace libtester

#endif // LIBTESTER_SRC_SERVERTEST_H
