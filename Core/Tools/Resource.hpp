#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "TypeNames.hpp"

struct Resource
{
    virtual ~Resource() = default;

    [[nodiscard]] const std::string_view& GetTypeName() const { return type_name; }
    [[nodiscard]] std::uint64_t GetID() const { return id; }

    template <typename ResourceType, typename... Args> static std::shared_ptr<ResourceType> Load(Args... args)
    {
        const std::uint64_t id = ResourceType::GetID(std::forward<Args>(args)...);

        const auto iterator = resources.find(id);
        if (iterator != resources.end()) return std::dynamic_pointer_cast<ResourceType>(iterator->second);

        auto resource_handle = std::make_shared<ResourceType>(std::forward<Args>(args)...);
        resource_handle->id = id;
        resource_handle->type_name = GetName<ResourceType>();

        resources[id] = resource_handle;
        return resource_handle;
    }

  private:
    friend struct FileResource;

    inline static std::unordered_map<std::uint64_t, std::shared_ptr<Resource>> resources;

    std::string_view type_name{};
    std::uint64_t id{};
};

struct FileResource : Resource
{
    [[nodiscard]] const std::string& GetPath() const { return path; }

    template <typename ResourceType, typename... Args>
    static std::shared_ptr<ResourceType> Load(const std::string& path, Args... args)
    {
        constexpr std::hash<std::string> hasher;
        const std::uint64_t id = hasher(path);

        const auto iterator = resources.find(id);
        if (iterator != resources.end())
            return std::dynamic_pointer_cast<ResourceType>(std::dynamic_pointer_cast<FileResource>(iterator->second));

        auto resource_handle =
            std::make_shared<ResourceType>(std::forward<const std::string&>(path), std::forward<Args>(args)...);
        resource_handle->id = id;
        resource_handle->type_name = GetName<ResourceType>();

        resources[id] = resource_handle;
        return resource_handle;
    }

  private:
    std::string path{};
};