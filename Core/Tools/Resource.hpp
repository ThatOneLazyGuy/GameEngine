#pragma once

#include <memory>
#include <unordered_map>

#include "TypeNames.hpp"

struct Resource
{
    virtual ~Resource() = default;

    [[nodiscard]] const std::string_view& GetTypeName() const { return type_name; }
    [[nodiscard]] std::uint64_t GetID() const { return id; }

    template <typename ResourceType, typename... Args>
    static std::shared_ptr<ResourceType> Load(Args... args);

    template <typename ResourceType>
    [[nodiscard]] static std::shared_ptr<ResourceType> Find(std::uint64_t id);

    template <typename ResourceType, typename... Args>
    [[nodiscard]] static std::shared_ptr<ResourceType> Find(Args... args);

    template <typename ResourceType>
    [[nodiscard]] static std::vector<std::shared_ptr<ResourceType>> GetResources();

  private:
    friend struct FileResource;

    inline static std::unordered_map<std::uint64_t, std::shared_ptr<Resource>> resources;

    std::string_view type_name{};
    std::uint64_t id{};
};

template <typename ResourceType, typename... Args>
std::shared_ptr<ResourceType> Resource::Load(Args... args)
{
    const std::uint64_t id = ResourceType::GetID(std::forward<Args>(args)...);

    const auto existing_resource = Resource::Find<ResourceType>(id);
    if (existing_resource) return existing_resource;

    auto resource_handle = std::make_shared<ResourceType>(std::forward<Args>(args)...);
    resource_handle->Resource::id = id;
    resource_handle->Resource::type_name = GetName<ResourceType>();

    resources[id] = resource_handle;
    return resource_handle;
}

template <typename ResourceType>
std::shared_ptr<ResourceType> Resource::Find(const std::uint64_t id)
{
    const auto iterator = resources.find(id);
    if (iterator != resources.end()) return std::dynamic_pointer_cast<ResourceType>(iterator->second);

    return nullptr;
}

template <typename ResourceType, typename... Args>
std::shared_ptr<ResourceType> Resource::Find(Args... args)
{
    const std::uint64_t id = ResourceType::GetID(std::forward<Args>(args)...);
    return Find<ResourceType>(id);
}

template <typename ResourceType = Resource>
std::vector<std::shared_ptr<ResourceType>> Resource::GetResources()
{
    std::vector<std::shared_ptr<ResourceType>> return_resources;
    for (const auto& [id, resource] : resources)
    {
        const auto valid_resource = std::dynamic_pointer_cast<ResourceType>(resource);
        if (valid_resource) return_resources.push_back(valid_resource);
    }

    return std::move(return_resources);
}

struct FileResource : Resource
{
    [[nodiscard]] const std::string& GetPath() const { return path; }

    template <typename ResourceType, typename... Args>
    static std::shared_ptr<ResourceType> Load(const std::string& path, Args... args);
    template <typename ResourceType>
    static std::shared_ptr<ResourceType> Find(const std::string& path);

  private:
    std::string path{};
};

template <typename ResourceType, typename... Args>
std::shared_ptr<ResourceType> FileResource::Load(const std::string& path, Args... args)
{
    constexpr std::hash<std::string> hasher;
    const std::uint64_t id = hasher(path);

    const auto existing_resource = Resource::Find<ResourceType>(id);
    if (existing_resource) return existing_resource;

    auto resource_handle =
        std::make_shared<ResourceType>(std::forward<const std::string&>(path), std::forward<Args>(args)...);
    resource_handle->Resource::id = id;
    resource_handle->Resource::type_name = GetName<ResourceType>();
    resource_handle->FileResource::path = path;

    resources[id] = resource_handle;
    return resource_handle;
}

template <typename ResourceType>
std::shared_ptr<ResourceType> FileResource::Find(const std::string& path)
{
    constexpr std::hash<std::string> hasher;
    const std::uint64_t id = hasher(path);

    return Resource::Find<ResourceType>(id);
}