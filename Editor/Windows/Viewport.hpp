#pragma once

#include "WindowBase.hpp"

class Viewport : public EditorWindow<Viewport>
{
  public:
    Viewport();
    ~Viewport() override = default;

    void Display() override;
};