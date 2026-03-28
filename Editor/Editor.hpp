#pragma once

#include "FileWatcher.hpp"

#include <ECS.hpp>
#include <ResourceManager.hpp>
#include <Tools/UniqueVector.hpp>

#include <mutex>

class AssetRegistry final : public AssetRegistryBase
{
  public:
    class ImportInfo
    {
      public:
        ImportInfo(std::string path);
        ImportInfo(std::string path, std::vector<UUID> assets);

        std::string path;
        std::vector<UUID> derived_assets; // UUIDs of the assets that were created when importing this file.

      private:
        FileWatcher::Watcher watcher;
    };

    AssetRegistry();
    ~AssetRegistry() override;

    void Update();

    template <ResourceType Type>
    UUID RegisterPath(const std::string& path)
    {
        UUID uuid = GetAssetUUID(path);
        if (uuid != NULL_UUID) return uuid;

        uuid = UUIDGenerator::Generate();
        asset_mapping.emplace(uuid, AssetInfo{path, Type::GetID().data()});

        return uuid;
    }

    void Import(const std::string& path);

    [[nodiscard]] const std::vector<ImportInfo>& GetImportInfos() const { return imported_files; }
    [[nodiscard]] const std::string& GetAssetPath(const UUID& uuid) const { return asset_mapping.at(uuid).path; }
    // Less efficient search due to searching the asset_mapping in the opposite direction.
    [[nodiscard]] const UUID& GetAssetUUID(const std::string& path) const;

  private:
    struct AssetInfo
    {
        std::string path;
        std::string type_id;
    };

    static void ImportedFileChanged(const std::string& path, FileWatcher::Event event);

    void RegisterImportedFile(const std::string& path, std::vector<UUID>&& assets);
    std::vector<UUID> UnregisterImportedFile(const std::string& path);

    // Inherited from AssetRegistryBase.
    [[nodiscard]] std::string_view GetAssetTypeID(const UUID& uuid) const override;
    [[nodiscard]] bool IsValidAsset(const UUID& uuid) const override { return asset_mapping.contains(uuid); }

    [[nodiscard]] std::ifstream GetAssetTextStream(const UUID& uuid) const override;
    [[nodiscard]] Files::BinaryReadStream GetAssetDataStream(const UUID& uuid) const override;

    [[nodiscard]] std::string GetAssetText(const UUID& uuid) const override;
    [[nodiscard]] std::vector<uint8> GetAssetData(const UUID& uuid) const override;

    inline static std::vector<std::pair<std::string, FileWatcher::Event>> changed_paths;
    inline static std::mutex changed_files_mutex;

    std::map<UUID, AssetInfo> asset_mapping;
    std::vector<ImportInfo> imported_files;
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