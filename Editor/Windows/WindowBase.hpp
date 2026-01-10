#pragma once

#include <functional>
#include <string>
#include <vector>

class EditorWindowBase;
using EditorWindowCreator = EditorWindowBase* (*)();

class EditorWindowBase
{
    template <typename>
    friend class EditorWindow;

  public:
    static const std::vector<EditorWindowCreator>& GetWindowCreators() { return window_creators; }

    virtual ~EditorWindowBase() = default;

    virtual void DisplayMenuBar() {}
    virtual void Display() = 0;

    [[nodiscard]] const std::string_view& GetName() const { return name; }
    [[nodiscard]] int GetWindowFlags() const { return window_flags; }
    bool is_open{true};

  protected:
    std::string_view name;
    int window_flags{};
    bool default_open{false};

  private:
    inline static std::vector<EditorWindowCreator> window_creators;

    EditorWindowBase() = default;
};

template <typename Self>
class EditorWindow : public EditorWindowBase
{
    friend Self;

  private:
    EditorWindow() = default;

    static EditorWindowBase* WindowCreator() { return new Self{}; }
    inline static const EditorWindowCreator register_func = window_creators.emplace_back(&WindowCreator);
};