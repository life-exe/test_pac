#include <cstdlib>
#include <fstream>
#include <print>

#include <nlohmann/json.hpp>

int main()
{
    std::ifstream file("config.json");

    if (!file)
    {
        std::println(stderr, "Failed to open config.json");
        return EXIT_FAILURE;
    }

    try
    {
        const auto config = nlohmann::json::parse(file);

        const std::string title = config["title"].get<std::string>();
        const int width = config["window"]["width"].get<int>();
        const int height = config["window"]["height"].get<int>();

        std::println("Hello, {}! Window: {}x{}", title, width, height);
    }
    catch (const std::exception& e)
    {
        std::println(stderr, "Failed to parse config.json, what: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}