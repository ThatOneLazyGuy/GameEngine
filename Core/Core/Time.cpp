#include "Time.hpp"

#include <chrono>

namespace
{
    using Timer = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using seconds =  std::chrono::duration<float>;

    TimePoint last_time{Timer::now()};
    TimePoint current_time{last_time};

    float delta_time{0.0f};
} // namespace

namespace Time
{
    void Update()
    {
        last_time = current_time;
        current_time = Timer::now();

        delta_time = std::chrono::duration_cast<seconds>(current_time - last_time).count();
    }

    float GetDeltaTime() { return delta_time; }
} // namespace Time