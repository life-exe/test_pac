#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <print>

#include "Config.h"

int main()
{
    try
    {
        std::ifstream file("config.json");
        if (!file)
        {
            std::println(stderr, "config.json not found in the current directory");
            return EXIT_FAILURE;
        }

        const LifeExe::GameConfig config = LifeExe::parseConfig(file);
        std::println("Hello, {}! Window: {}x{}", config.title, config.width, config.height);
    }
    catch (const std::exception& e)
    {
        // Handlers use fputs: it cannot throw, so no exception escapes main.
        std::fputs(e.what(), stderr);
        std::fputs("\n", stderr);
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::fputs("unknown error\n", stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}