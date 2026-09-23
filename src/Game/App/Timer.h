#pragma once

#include <chrono>

namespace LifeExe::App
{

class Timer
{
public:
    Timer() : m_lastTime{std::chrono::steady_clock::now()} {}

    float tick()
    {
        const auto now{std::chrono::steady_clock::now()};
        const std::chrono::duration<float> elapsed{now - m_lastTime};
        m_lastTime = now;
        return elapsed.count();
    }

    void reset() { m_lastTime = std::chrono::steady_clock::now(); }

private:
    std::chrono::steady_clock::time_point m_lastTime;
};

}  // namespace LifeExe::App
