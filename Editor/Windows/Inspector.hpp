#pragma once

#include "WindowBase.hpp"

class Inspector : public EditorWindow<Inspector>
{
  public:
    Inspector();
    ~Inspector() override = default;

    void Display() override;
};