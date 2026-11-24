#pragma once

#include <cstddef>
#include <random>

namespace LifeExe::Core
{

class IRandomSource
{
public:
    virtual ~IRandomSource() = default;
    virtual std::size_t nextIndex(std::size_t count) = 0;
};

class DefaultRandomSource final : public IRandomSource
{
public:
    DefaultRandomSource() : m_engine{std::random_device{}()} {}
    explicit DefaultRandomSource(std::mt19937::result_type seed) : m_engine{seed} {}

    std::size_t nextIndex(std::size_t count) override
    {
        if (count <= 1)
        {
            return 0;
        }
        std::uniform_int_distribution<std::size_t> dist{0, count - 1};
        return dist(m_engine);
    }

private:
    std::mt19937 m_engine;
};

}  // namespace LifeExe::Core
