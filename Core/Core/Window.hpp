#pragma once

#include <functional>
#include <Tools/Types.hpp>

namespace Window
{
    void Init(const std::function<void(const void*)>& event_process_func);
    void Exit();

    bool PollEvents();

    sint32 GetWidth();
    sint32 GetHeight();

    void* GetHandle();


} // namespace Window