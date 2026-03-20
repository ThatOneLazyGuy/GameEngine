#pragma once

#include "FileWatcher.hpp"

#include <ECS.hpp>
#include <ResourceManager.hpp>
#include <Tools/UniqueVector.hpp>

class AssetRegistry : public AssetRegistryBase
{
  public:
    struct ImportInfo
    {
        std::string path;
        FileWatcher::Watcher watcher;
        std::vector<UUID> derived_assets; // UUIDs of the assets that were created when importing this file.
    };

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

    void Import(const std::string& path);

    [[nodiscard]] const std::vector<ImportInfo>& GetImportInfos() const { return imported_files; }
    [[nodiscard]] const std::map<std::string, UUID>& GetAssetFileMapping() const { return reverse_mapping; }
    [[nodiscard]] const std::string& GetAssetPath(const UUID& uuid) const { return mapping.at(uuid).path; }

  private:
    struct AssetInfo
    {
        std::string path;
        std::string type_id;
    };

    std::map<UUID, AssetInfo> mapping;
    std::map<std::string, UUID> reverse_mapping;

    std::vector<ImportInfo> imported_files;

    void RegisterImportedFile(const std::string& path, std::vector<UUID>&& assets);
    std::vector<UUID> UnregisterImportedFile(const std::string& path);

    static void ImportedFileChanged(const std::string& path, FileWatcher::Event event);

    // Functions inherited from AssetRegistryBase.
    [[nodiscard]] std::string_view GetAssetTypeID(const UUID& uuid) const override;
    [[nodiscard]] bool IsValidAsset(const UUID& uuid) const override { return mapping.contains(uuid); }

    [[nodiscard]] std::ifstream GetAssetTextStream(const UUID& uuid) const override;
    [[nodiscard]] Files::BinaryReadStream GetAssetDataStream(const UUID& uuid) const override;
    [[nodiscard]] std::string GetAssetText(const UUID& uuid) const override;
    [[nodiscard]] std::vector<uint8> GetAssetData(const UUID& uuid) const override;
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