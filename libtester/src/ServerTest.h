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

class ServerTestConfig
{
public:
    ServerTestConfig(const std::chrono::milliseconds& testTime,
        const std::chrono::milliseconds& bootTime,
        const std::string& programsPath);

    std::chrono::milliseconds GetTestTime() const;
    std::chrono::milliseconds GetBootTime() const;
    std::string GetProgramsPath() const;

private:
    std::chrono::milliseconds mTestTime;
    std::chrono::milliseconds mBootTime;
    std::string mProgramsPath;
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
        _task->Run();

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
        _task->Run();

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
        const std::shared_ptr<libtester::TimedTask>& _task)
    {
        libcomp::DayCare procManager(false);

        ASSERT_TRUE(procManager.DetainMonsters(_programsPath));

        std::this_thread::sleep_for(_bootDur);

        _task->Run();

        procManager.CloseDoors();
        procManager.WaitForExit();

        finished.set_value(true);
    }, std::ref(promisedFinished), config.GetBootTime(),
        config.GetProgramsPath(), task).detach();

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
        std::chrono::milliseconds(10000),
        std::chrono::milliseconds(3000),
        "bin/testing/programs-lobby.xml");
}

} // namespace ServerConfig

} // namespace libtester

#endif // LIBTESTER_SRC_SERVERTEST_H
