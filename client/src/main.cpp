// libtester Includes
#include <ChannelClient.h>

// libcomp Includes
#include <Log.h>
#include <ScriptEngine.h>

// Standard C++ Includes
#include <iostream>

// Standard C Includes
#include <cstdlib>
#include <cstdio>

static bool gRunning = true;
static int gReturnCode = EXIT_SUCCESS;

static void ScriptExit(int returnCode)
{
    gReturnCode = returnCode;
    gRunning = false;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    // Enable the log so it prints to the console.
    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    // Create the script engine.
    libcomp::ScriptEngine engine(true);

    // Register the exit function.
    Sqrat::RootTable(engine.GetVM()).Func("exit", ScriptExit);

    // Register the client testing classes.
    engine.Using<libtester::ChannelClient>();

    libcomp::String code, script;

    std::cout << "sq> ";

    int depth = 0;

    while(gRunning)
    {
        char c = (char)std::getchar();

        code += libcomp::String(&c, 1);

        switch(c)
        {
            case '{':
            {
                depth++;
                break;
            }
            case '}':
            {
                depth--;
                break;
            }
            case '\n':
            {
                if(0 == depth)
                {
                    engine.Eval(code);
                    script += code;
                    code.Clear();

                    if(gRunning)
                    {
                        std::cout << "sq> ";
                    }
                }

                break;
            }
        }
    }

    std::cout << "Final script: " << std::endl << script.C();

    return gReturnCode;
}
