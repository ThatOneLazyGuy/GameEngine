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
        const auto& path_iterator = reverse_mapping.find(path);
        if (path_iterator != reverse_mapping.end()) return path_iterator->second;

        const UUID uuid = UUIDGenerator::Generate();
        const auto& iterator = mapping.emplace(uuid, path);
        reverse_mapping.emplace(iterator.first->second, uuid);

        return uuid;
    }

    const std::map<std::string_view, UUID>& GetAssetFileMapping() const { return reverse_mapping; }

  private:
    [[nodiscard]] bool IsValidResource(const UUID& uuid) const override { return mapping.contains(uuid); }

    [[nodiscard]] std::string GetResourceText(const UUID& uuid) const override;
    [[nodiscard]] std::vector<uint8> GetResourceData(const UUID& uuid) const override;

    std::map<UUID, std::string> mapping;
    std::map<std::string_view, UUID> reverse_mapping;
};

namespace Editor
{
    inline AssetRegistry* asset_registry{nullptr};

    struct EditorOnly
    {
    };

    ECS::Entity CreateEntity(std::string name = {});

    inline UniqueVector<ECS::Entity, ECS::EntityHasher> selected_entities;

} // namespace Editor