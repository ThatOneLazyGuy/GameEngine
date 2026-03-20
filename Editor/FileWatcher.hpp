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
        Watcher(void* handle, std::string path) : handle{handle}, path{std::move(path)} {}
        ~Watcher();

        Watcher(Watcher&& other) noexcept;
        Watcher& operator=(Watcher&& other) noexcept;

        [[nodiscard]] bool Valid() const { return handle != nullptr; }
        [[nodiscard]] const std::string& GetPath() const { return path; }

      private:
        void* handle{nullptr};
        std::string path;
    };

    Watcher CreateWatcher(const std::string& path, const std::function<void(const std::string&, Event)>& callback);

} // namespace FileWatcher