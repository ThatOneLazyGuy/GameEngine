#pragma once

#include "WindowBase.hpp"

class Hierarchy : public EditorWindow<Hierarchy>
{
  public:
    Hierarchy();
    ~Hierarchy() override = default;

    void DisplayMenuBar() override;
    void Display() override;
};