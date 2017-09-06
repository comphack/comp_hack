/**
 * @file libcomp/tests/ScriptEngine.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test the Squirrel scripting language interface.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2016 COMP_hack Team <compomega@tutanota.com>
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

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <sqrat.h>
#include <PopIgnore.h>

#include <Log.h>
#include <Packet.h>
#include <ScriptEngine.h>
#include <TestObject.h>

using namespace libcomp;

TEST(ScriptEngine, EvalCompileError)
{
    ScriptEngine engine;

    int errorCount = 0;

    // Check for a compile error that produces a log message.
    Log::GetSingletonPtr()->AddLogHook(
        [](Log::Level_t level, const String& msg, void *pUserData)
        {
            (void)msg;

            EXPECT_EQ(level, Log::LOG_LEVEL_ERROR);

            (*reinterpret_cast<int*>(pUserData))++;
        }, &errorCount);

    EXPECT_FALSE(engine.Eval("1=2"));
    EXPECT_EQ(errorCount, 1);

    Log::GetSingletonPtr()->ClearHooks();
}

TEST(ScriptEngine, EvalRuntimeError)
{
    ScriptEngine engine;

    int errorCount = 0;

    // Check for a runtime error that produces log messages.
    Log::GetSingletonPtr()->AddLogHook(
        [](Log::Level_t level, const String& msg, void *pUserData)
        {
            (void)msg;

            EXPECT_EQ(level, Log::LOG_LEVEL_ERROR);

            (*reinterpret_cast<int*>(pUserData))++;
        }, &errorCount);

    EXPECT_FALSE(engine.Eval("FunctionThatDoesNotExist()"));
    EXPECT_NE(errorCount, 0);

    Log::GetSingletonPtr()->ClearHooks();
}

TEST(ScriptEngine, EvalPrint)
{
    ScriptEngine engine;

    int messageCount = 0;

    // Check for a call to print() that produces a log message.
    Log::GetSingletonPtr()->AddLogHook(
        [](Log::Level_t level, const String& msg, void *pUserData)
        {
            EXPECT_EQ(msg, "SQUIRREL: Test\n");
            EXPECT_EQ(level, Log::LOG_LEVEL_INFO);

            (*reinterpret_cast<int*>(pUserData))++;
        }, &messageCount);

    EXPECT_TRUE(engine.Eval("print(\"Test\");"));
    EXPECT_EQ(messageCount, 1);

    Log::GetSingletonPtr()->ClearHooks();
}

TEST(ScriptEngine, ReadOnlyPacket)
{
    String scriptMessages;

    Log::GetSingletonPtr()->AddLogHook(
        [&scriptMessages](Log::Level_t level, const String& msg)
        {
            (void)level;

            scriptMessages += msg;
        });

    ScriptEngine engine;
    engine.Using<libcomp::Packet>();

    EXPECT_TRUE(engine.Eval(
        "p <- Packet();\n"
        "p.WriteBlank(3);\n"
        "print(p.Size());\n"
    ));
    EXPECT_EQ(scriptMessages, "SQUIRREL: 3\n");
    scriptMessages.Clear();

    EXPECT_TRUE(engine.Eval(
        "p <- Packet();\n"
        "print(p.Size());\n"
    ));
    EXPECT_EQ(scriptMessages, "SQUIRREL: 0\n");
    scriptMessages.Clear();

    Log::GetSingletonPtr()->ClearHooks();
}

TEST(ScriptEngine, ReadWriteArray)
{
    String scriptMessages;

    Log::GetSingletonPtr()->AddLogHook(
        [&scriptMessages](Log::Level_t level, const String& msg)
        {
            (void)level;

            scriptMessages += msg;
        });

    ScriptEngine engine;
    engine.Using<libcomp::Packet>();

    EXPECT_TRUE(engine.Eval(
        "p <- Packet();\n"
        "local b = blob();\n"
        "b.writen(-1095041334, 'i');" // 0xCAFEBABE
        "p.WriteArray(b);\n"
        "if(4 == p.Size())\n"
        "{\n"
            "p.Rewind(4)\n"
            "local c = p.ReadArray(4);\n"
            "print(c.readn('i'));\n"
        "}\n"
    ));
    EXPECT_EQ(scriptMessages, "SQUIRREL: -1095041334\n");
    scriptMessages.Clear();

    Log::GetSingletonPtr()->ClearHooks();
}

TEST(ScriptEngine, FunctionCall)
{
    String scriptMessages;

    Log::GetSingletonPtr()->AddLogHook(
        [&scriptMessages](Log::Level_t level, const String& msg)
        {
            (void)level;

            scriptMessages += msg;
        });

    ScriptEngine engine;
    engine.Using<libcomp::Packet>();

    EXPECT_TRUE(engine.Eval(
        "function TestFunction(a)\n"
        "{\n"
            "b <- Packet();\n"
            "a.WriteU16Little(0x1234);\n"
            "b.WriteU16Little(0x5678);\n"
            "return b;\n"
        "}\n"
    ));

    std::shared_ptr<Sqrat::ObjectReference<libcomp::Packet>> ref;
    std::shared_ptr<libcomp::Packet> a(new libcomp::Packet);
    std::shared_ptr<libcomp::Packet> b;

    ref = Sqrat::RootTable(engine.GetVM()).GetFunction("TestFunction"
        ).Evaluate<Sqrat::ObjectReference<libcomp::Packet>>(a);

    EXPECT_EQ(scriptMessages, "");

    ASSERT_TRUE(ref);

    b = ref->GetSharedObject();

    ASSERT_TRUE(b);

    EXPECT_EQ(b->Size(), 2);
    EXPECT_EQ(a->Size(), 2);

    EXPECT_EQ(b->Tell(), 2);
    EXPECT_EQ(a->Tell(), 2);

    a->Rewind();
    b->Rewind();

    EXPECT_EQ(a->ReadU16Little(), 0x1234);
    EXPECT_EQ(b->ReadU16Little(), 0x5678);

    scriptMessages.Clear();

    Log::GetSingletonPtr()->ClearHooks();
}

TEST(ScriptEngine, GeneratedObject)
{
    String scriptMessages;

    Log::GetSingletonPtr()->AddLogHook(
        [&scriptMessages](Log::Level_t level, const String& msg)
    {
        (void)level;

        scriptMessages += msg;
    });

    ScriptEngine engine;
    engine.Using<objects::TestObject>();

    EXPECT_TRUE(engine.Eval(
        "t <- TestObject();\n"
        "if(t.GetUnsigned8() == 100)\n"
        "{\n"
        "   error(\"Test value already set!\");\n"
        "}\n"
        "if(!t.SetUnsigned8(256))\n"       //Should be out of bounds and fail
        "{\n"
        "   t.SetUnsigned8(100);\n"
        "}\n"
        "print(t.GetUnsigned8());\n"
        "print(t.GetStringCP932());\n"
        "t.SetStringCP932(\"日本人\");\n"
        "print(t.GetStringCP932());\n"
        ));
    EXPECT_EQ(scriptMessages, "SQUIRREL: 100\n"
        "SQUIRREL: 日本語\n"
        "SQUIRREL: 日本人\n");
    scriptMessages.Clear();

    Log::GetSingletonPtr()->ClearHooks();
}

int main(int argc, char *argv[])
{
    try
    {
        ::testing::InitGoogleTest(&argc, argv);

        return RUN_ALL_TESTS();
    }
    catch(...)
    {
        return EXIT_FAILURE;
    }
}
