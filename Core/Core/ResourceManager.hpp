#pragma once

#include <map>
#include <memory>
#include <cassert>
#include <concepts>

#include "Tools/Types.hpp"
#include "Tools/Logging.hpp"
#include "Tools/uuid.hpp"

class ResourceBase
{
  public:
    virtual ~ResourceBase() = default;

    [[nodiscard]] virtual usize GetTypeHash() const = 0;

  private:
    ResourceBase() = default;

    template <typename Derived>
    friend class Resource;
};

template <typename Derived>
class Resource : public ResourceBase
{
  public:
    ~Resource() override = default;

  protected:
    Resource() = default;

    [[nodiscard]] usize GetTypeHash() const final { return typeid(Derived).hash_code(); }
};

namespace ResourceManager
{

    inline std::map<UUID, std::pair<usize, std::unique_ptr<ResourceBase>>> resources{};

}

// Handle to the resource in the resource manager, template parameter should be the derived resource's type (CRTP).
template <typename Derived>
requires std::derived_from<Derived, Resource<Derived>>
class ResourceRef
{
  public:
    ResourceRef() = default;
    ResourceRef(const UUID& uuid) : uuid{uuid}
    {
        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator == ResourceManager::resources.end())
        {
            Log::Error("Tried to create ResourceRef with invalid resource id.");
            return;
        }

        auto&& [reference_count, value] = iterator->second;
        assert(value->GetTypeHash() == type_hash && "id type doesn't match ResourceRef type.");

        ++reference_count;
    }
    ~ResourceRef()
    {
        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator == ResourceManager::resources.end()) return;

        usize& reference_count = iterator->second.first;
        --reference_count;
    }

    ResourceRef(const ResourceRef& other)
    {
        uuid = other.uuid;

        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator != ResourceManager::resources.end())
        {
            usize& reference_count = iterator->second.first;
            ++reference_count;
        }
    }

    ResourceRef(ResourceRef&& other) noexcept : uuid{other.uuid} { other.uuid = NULL_UUID; }
    ResourceRef(const ResourceRef&& other) noexcept : ResourceRef{other} {}

    ResourceRef& operator=(const ResourceRef& other)
    {
        Clear();

        uuid = other.uuid;

        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator != ResourceManager::resources.end())
        {
            usize& reference_count = iterator->second.first;
            ++reference_count;
        }

        return *this;
    }

    ResourceRef& operator=(ResourceRef&& other) noexcept
    {
        Clear();

        uuid = other.uuid;
        other.uuid = NULL_UUID;

        return *this;
    }

    Derived* operator->() { return static_cast<Derived*>(ResourceManager::resources[uuid].second.get()); }
    const Derived* operator->() const { return static_cast<const Derived*>(ResourceManager::resources[uuid].second.get()); }
    Derived& operator*() { return static_cast<Derived&>(*ResourceManager::resources[uuid].second); }
    const Derived& operator*() const { return static_cast<const Derived&>(*ResourceManager::resources.at(uuid).second); }

    [[nodiscard]] bool Valid() const { return ResourceManager::resources.contains(uuid); }
    void Clear()
    {
        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator != ResourceManager::resources.end())
        {
            usize& reference_count = iterator->second.first;
            --reference_count;
        }

        uuid = NULL_UUID;
    }

  private:
    inline static usize type_hash{typeid(Derived).hash_code()};

    UUID uuid{NULL_UUID};
};

namespace ResourceManager
{
    inline std::map<std::string, UUID> permanent_mapping;

    // Concept to check if the resource defines a specific GetID function with different parameters.
    template <typename ResourceType, typename... Args>
    concept HasExplictGetID = requires(const std::string& path, Args&&... args) { ResourceType::GetID(path, std::forward<Args>(args)...); };

    template <typename ResourceType, typename... Args>
    requires std::derived_from<ResourceType, Resource<ResourceType>>
    ResourceRef<ResourceType> Load(const std::string& path, Args&&... args)
    {
        std::string id_string;
        if constexpr (HasExplictGetID<ResourceType, Args...>) id_string = ResourceType::GetID(path, std::forward<Args>(args)...);
        else id_string = path;

        UUID uuid;

        const auto permanent_iterator = permanent_mapping.find(id_string);
        if (permanent_iterator != permanent_mapping.end()) uuid = permanent_iterator->second;
        else
        {
            uuid = UUIDGenerator::Generate();
            permanent_mapping.emplace(id_string, uuid);
        }


        if (resources.contains(uuid)) return ResourceRef<ResourceType>{uuid};

        resources[uuid] = std::pair{0, std::make_unique<ResourceType>(path, std::forward<Args>(args)...)};

        return ResourceRef<ResourceType>{uuid};
    }

    void Init();
    void Exit();

    void Update();

} // namespace ResourceManager