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
    static constexpr bool IsTextConstructable{false};

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

    [[nodiscard]] const UUID& GetUUID() const { return uuid; }

  private:
    inline static usize type_hash{typeid(Derived).hash_code()};

    UUID uuid{NULL_UUID};
};

class AssetRegistryBase
{
  protected:
    AssetRegistryBase() = default;

  public:
    virtual ~AssetRegistryBase() = default;

    [[nodiscard]] virtual bool IsValidResource(const UUID& uuid) const = 0;

    [[nodiscard]] virtual std::string GetResourceText(const UUID& uuid) const = 0;
    [[nodiscard]] virtual std::vector<uint8> GetResourceData(const UUID& uuid) const = 0;
};

namespace ResourceManager
{
    inline std::unique_ptr<AssetRegistryBase> asset_registry;

    template <typename ResourceType>
    requires std::derived_from<ResourceType, Resource<ResourceType>>
    ResourceRef<ResourceType> Load(const UUID& uuid)
    {
        if (resources.contains(uuid)) return ResourceRef<ResourceType>{uuid};

        if (!asset_registry->IsValidResource(uuid))
        {
            Log::Error("UUID isn't loaded and is not a registered asset: {}", uuid.str());
            return ResourceRef<ResourceType>{};
        }


        if constexpr (ResourceType::IsTextConstructable)
        {
            const std::string& text = asset_registry->GetResourceText(uuid);
            resources[uuid] = std::pair{0, std::make_unique<ResourceType>(text)};
        }
        else
        {
            const std::vector<uint8>& data = asset_registry->GetResourceData(uuid);
            resources[uuid] = std::pair{0, std::make_unique<ResourceType>(data)};
        }

        return ResourceRef<ResourceType>{uuid};
    }

    template <typename ResourceType, typename... Args>
    requires std::derived_from<ResourceType, Resource<ResourceType>>
    ResourceRef<ResourceType> Create(Args&&... args)
    {
        const UUID& uuid = UUIDGenerator::Generate();
        resources[uuid] = std::pair{0, std::make_unique<ResourceType>(std::forward<Args>(args)...)};

        return ResourceRef<ResourceType>{uuid};
    }

    template <typename RegistryType>
    requires std::derived_from<RegistryType, AssetRegistryBase>
    RegistryType* Init()
    {
        RegistryType* new_registry = new RegistryType;
        asset_registry = std::unique_ptr<RegistryType>(new_registry);

        return new_registry;
    }

    void Exit();

    void Update();

} // namespace ResourceManager