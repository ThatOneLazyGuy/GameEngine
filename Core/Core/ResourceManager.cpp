#include "ResourceManager.hpp"

#include "Tools/Files.hpp"

namespace ResourceManager
{

    usize GetResourceTypeHash(const UUID& uuid)
    {
        const auto iterator = resources.find(uuid);
        if (iterator != resources.end()) return iterator->second.second->GetTypeHash();

        if (!asset_registry->IsValidAsset(uuid))
        {
            Log::Error("UUID isn't loaded and is not a registered asset: {}", uuid.str());
            return 0;
        }

        return asset_registry->GetAssetMetadata(uuid);
    }

    void Exit()
    {
        resources.clear();
        asset_registry.reset();
    }

    void Update()
    {
        std::erase_if(resources, [](const std::pair<const UUID, std::pair<usize, std::unique_ptr<ResourceBase>>>& element) {
            return element.second.first == 0;
        });
    }

} // namespace ResourceManager