#include "FileWatcher.hpp"

#include <filesystem>
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
        // The Filewatch library for some reason doesn't actually return the file path in the callback, but only the name, so we pass in the file path manually into the lambda.
        const auto backend_callback = [callback, path](const std::string&, const filewatch::Event event) {
            callback(path, ConvertEvent(event));
        };

        Watcher watcher{
            new filewatch::FileWatch<std::string>{std::filesystem::absolute(path).generic_string(), backend_callback},
        };

        return watcher;
    }
} // namespace FileWatcher