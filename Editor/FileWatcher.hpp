#pragma once

#include <functional>
#include <Types.hpp>

#include <string>

namespace FileWatcher
{
    enum Event : uint8
    {
        ADDED,
        REMOVED,
        MODIFIED,
        RENAMED_OLD,
        RENAMED_NEW
    };

    class Watcher
    {
      public:
        Watcher() = default;
        Watcher(void* handle) : handle{handle} {}
        ~Watcher();

        Watcher(Watcher&& other) noexcept;
        Watcher& operator=(Watcher&& other) noexcept;

        [[nodiscard]] bool Valid() const { return handle != nullptr; }

      private:
        void* handle{nullptr};
    };

    Watcher CreateWatcher(const std::string& path, const std::function<void(const std::string&, Event)>& callback);

} // namespace FileWatcher