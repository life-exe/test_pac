#pragma once

#include "GameSnapshot.h"

namespace LifeExe::Presentation
{

class ConsoleRenderer
{
public:
    void render(const Core::GameSnapshot& snapshot) const;
};

}  // namespace LifeExe::Presentation