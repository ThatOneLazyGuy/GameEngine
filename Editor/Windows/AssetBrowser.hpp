#pragma once

#include "WindowBase.hpp"

class AssetBrowser : public EditorWindow<AssetBrowser>
{
  public:
    AssetBrowser();
    ~AssetBrowser() override = default;

    void Display() override;
};