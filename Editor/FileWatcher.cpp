#include "FileWatcher.hpp"

#include <FileWatch.hpp>

namespace
{
    FileWatcher::Event ConvertEvent(const filewatch::Event event)
    {
        switch (event)
        {
        case filewatch::Event::added:
            return FileWatcher::ADDED;

        case filewatch::Event::removed:
            return FileWatcher::REMOVED;

        case filewatch::Event::modified:
            return FileWatcher::MODIFIED;

        case filewatch::Event::renamed_old:
            return FileWatcher::RENAMED_OLD;

        case filewatch::Event::renamed_new:
            return FileWatcher::RENAMED_NEW;
        }

        // Shouldn't happen, since all values of fileWatcher::Event are handled.
        return FileWatcher::ADDED;
    }
} // namespace

namespace FileWatcher
{
    Watcher::~Watcher() { delete static_cast<filewatch::FileWatch<std::string>*>(handle); }

    Watcher::Watcher(Watcher&& other) noexcept
    {
        if (Valid()) delete static_cast<filewatch::FileWatch<std::string>*>(handle);

        handle = other.handle;
        other.handle = nullptr;
    }

    Watcher& Watcher::operator=(Watcher&& other) noexcept
    {
        if (Valid()) delete static_cast<filewatch::FileWatch<std::string>*>(handle);

        handle = other.handle;
        other.handle = nullptr;

        return *this;
    }

    Watcher CreateWatcher(const std::string& path, const std::function<void(const std::string&, Event)>& callback)
    {
        const auto backend_callback = [callback](const std::string& path, const filewatch::Event event) {
            callback(path, ConvertEvent(event));
        };

        Watcher watcher{
            new filewatch::FileWatch<std::string>{path, backend_callback},
        };

        return watcher;
    }
} // namespace FileWatcher