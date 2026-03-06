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

template <typename>
class Resource : public ResourceBase
{
  public:
    ~Resource() override = default;

    static const usize type_hash;

  protected:
    Resource() = default;

    [[nodiscard]] usize GetTypeHash() const final { return type_hash; }
};

template <typename Derived>
const usize Resource<Derived>::type_hash{typeid(Derived).hash_code()};

namespace ResourceManager
{
    struct ResourceRefCount
    {
        usize reference_count;
        std::unique_ptr<ResourceBase> resource;
    };

    inline std::map<UUID, ResourceRefCount> resources{};

} // namespace ResourceManager

template <typename Type>
concept ResourceType = std::derived_from<Type, Resource<Type>>;

// Handle to the resource in the resource manager, template parameter should be the derived resource's type (CRTP).
template <ResourceType Type>
class ResourceRef
{
  public:
    using ResourceType = Type;

    ResourceRef() = default;
    ResourceRef(const UUID& uuid) : uuid{uuid}
    {
        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator == ResourceManager::resources.end())
        {
            Log::Error("Tried to create ResourceRef with invalid resource id.");
            return;
        }

        auto&& [reference_count, resource] = iterator->second;
        assert(resource->GetTypeHash() == ResourceType::type_hash && "id type doesn't match ResourceRef type.");

        ++reference_count;
        pointer = static_cast<ResourceType*>(resource.get());
    }
    ~ResourceRef()
    {
        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator != ResourceManager::resources.end()) --iterator->second.reference_count;
    }

    ResourceRef(const ResourceRef& other)
    {
        uuid = other.uuid;
        pointer = other.pointer;

        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator == ResourceManager::resources.end()) return;

        ++iterator->second.reference_count;
    }

    ResourceRef(ResourceRef&& other) noexcept : uuid{other.uuid}, pointer{other.pointer}
    {
        other.uuid = NULL_UUID;
        other.pointer = nullptr;
    }
    ResourceRef(const ResourceRef&& other) noexcept : ResourceRef{other} {}

    ResourceRef& operator=(const ResourceRef& other)
    {
        if (this == &other) return *this;

        Clear();

        uuid = other.uuid;
        pointer = other.pointer;

        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator != ResourceManager::resources.end()) ++iterator->second.reference_count;

        return *this;
    }

    ResourceRef& operator=(ResourceRef&& other) noexcept
    {
        Clear();

        uuid = other.uuid;
        pointer = other.pointer;

        other.uuid = NULL_UUID;
        other.pointer = nullptr;

        return *this;
    }

    ResourceType* operator->() { return pointer; }
    const ResourceType* operator->() const { return pointer; }
    ResourceType& operator*() { return *pointer; }
    const ResourceType& operator*() const { return *pointer; }

    [[nodiscard]] bool operator==(const ResourceRef& other) const { return uuid == other.uuid; }
    [[nodiscard]] bool operator==(const ResourceType* other_pointer) const { return pointer == other_pointer; }
    [[nodiscard]] bool operator==(const UUID& other_uuid) const { return uuid == other_uuid; }
    [[nodiscard]] bool operator==(const std::nullptr_t) const { return pointer == nullptr; }

    [[nodiscard]] bool Valid() const { return ResourceManager::resources.contains(uuid); }
    void Clear()
    {
        const auto iterator = ResourceManager::resources.find(uuid);
        if (iterator != ResourceManager::resources.end()) --iterator->second.reference_count;

        uuid = NULL_UUID;
        pointer = nullptr;
    }

    [[nodiscard]] const UUID& GetUUID() const { return uuid; }

  private:
    UUID uuid{NULL_UUID};
    ResourceType* pointer{nullptr};
};

class AssetRegistryBase
{
  protected:
    AssetRegistryBase() = default;

  public:
    virtual ~AssetRegistryBase() = default;

    [[nodiscard]] virtual usize GetAssetMetadata(const UUID& uuid) const = 0;
    [[nodiscard]] virtual bool IsValidAsset(const UUID& uuid) const = 0;

    [[nodiscard]] virtual std::string GetAssetText(const UUID& uuid) const = 0;
    [[nodiscard]] virtual std::vector<uint8> GetAssetData(const UUID& uuid) const = 0;
};

namespace ResourceManager
{
    inline std::unique_ptr<AssetRegistryBase> asset_registry;

    template <ResourceType ResourceType>
    ResourceRef<ResourceType> Load(const UUID& uuid)
    {
        if (resources.contains(uuid)) return ResourceRef<ResourceType>{uuid};

        if (!asset_registry->IsValidAsset(uuid))
        {
            Log::Error("UUID isn't loaded and is not a registered asset: {}", uuid.str());
            return ResourceRef<ResourceType>{};
        }


        if constexpr (ResourceType::IsTextConstructable)
        {
            const std::string& text = asset_registry->GetAssetText(uuid);
            resources.emplace(uuid, ResourceRefCount{0, std::make_unique<ResourceType>(text)});
        }
        else
        {
            const std::vector<uint8>& data = asset_registry->GetAssetData(uuid);
            resources.emplace(uuid, ResourceRefCount{0, std::make_unique<ResourceType>(data)});
        }

        return ResourceRef<ResourceType>{uuid};
    }

    template <ResourceType ResourceType, typename... Args>
    ResourceRef<ResourceType> Create(Args&&... args)
    {
        const UUID& uuid = UUIDGenerator::Generate();
        resources.emplace(uuid, ResourceRefCount{0, std::make_unique<ResourceType>(std::forward<Args>(args)...)});

        return ResourceRef<ResourceType>{uuid};
    }

    [[nodiscard]] usize GetResourceTypeHash(const UUID& uuid);

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