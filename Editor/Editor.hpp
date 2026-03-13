#pragma once

#include <ECS.hpp>
#include <ResourceManager.hpp>
#include <Tools/UniqueVector.hpp>

class AssetRegistry : public AssetRegistryBase
{
  public:
    AssetRegistry();
    ~AssetRegistry() override;

    template <ResourceType Type>
    UUID RegisterPath(const std::string& path)
    {
        const auto& path_iterator = reverse_mapping.find(path);
        if (path_iterator != reverse_mapping.end()) return path_iterator->second;

        const UUID uuid = UUIDGenerator::Generate();
        const auto& iterator = mapping.emplace(uuid, AssetInfo{path, Type::GetID().data()});
        reverse_mapping.emplace(iterator.first->second.path, uuid);

        return uuid;
    }

    [[nodiscard]] const std::map<std::string, UUID>& GetAssetFileMapping() const { return reverse_mapping; }

  private:
    struct AssetInfo
    {
        std::string path;
        std::string type_id;
    };

    [[nodiscard]] std::string_view GetAssetTypeID(const UUID& uuid) const override;
    [[nodiscard]] bool IsValidAsset(const UUID& uuid) const override { return mapping.contains(uuid); }

    [[nodiscard]] std::ifstream GetAssetTextStream(const UUID& uuid) const override;
    [[nodiscard]] Files::BinaryReadStream GetAssetDataStream(const UUID& uuid) const override;
    [[nodiscard]] std::string GetAssetText(const UUID& uuid) const override;
    [[nodiscard]] std::vector<uint8> GetAssetData(const UUID& uuid) const override;

    std::map<UUID, AssetInfo> mapping;
    std::map<std::string, UUID> reverse_mapping;
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