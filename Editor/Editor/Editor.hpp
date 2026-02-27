#pragma once

#include <Core/ECS.hpp>
#include <Core/ResourceManager.hpp>
#include <Tools/UniqueVector.hpp>

class AssetRegistry : public AssetRegistryBase
{
  public:
    AssetRegistry();
    ~AssetRegistry() override;

    UUID RegisterPath(const std::string& path)
    {
        const UUID uuid = UUIDGenerator::Generate();
        const auto& iterator = mapping.emplace(uuid, path);
        reverse_mapping.emplace(iterator.first->second, uuid);

        return uuid;
    }

  private:
    [[nodiscard]] bool IsValidResource(const UUID& uuid) const override { return mapping.contains(uuid); }

    [[nodiscard]] std::string GetResourceText(const UUID& uuid) const override;
    [[nodiscard]] std::vector<uint8> GetResourceData(const UUID& uuid) const override;

    std::map<UUID, std::string> mapping;
    std::map<std::string_view, UUID> reverse_mapping;
};

namespace Editor
{

    struct EditorOnly
    {
    };

    ECS::Entity CreateEntity(std::string name = {});

    inline UniqueVector<ECS::Entity, ECS::EntityHasher> selected_entities;

} // namespace Editor