#pragma once

#include <functional>

namespace Window
{
    void Init(const std::function<void(const void*)>& event_process_func);
    void Exit();

    bool PollEvents();

    size_t GetWidth();
    size_t GetHeight();

    void* GetHandle();


} // namespace Window