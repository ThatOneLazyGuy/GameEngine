#include "ResourceManager.hpp"

#include "Files.hpp"

namespace ResourceManager
{

    std::string_view GetResourceTypeID(const UUID& uuid)
    {
        const auto iterator = resources.find(uuid);
        if (iterator != resources.end()) return iterator->second.resource->GetTypeID();

        if (!asset_registry->IsValidAsset(uuid))
        {
            Log::Error("UUID isn't loaded and is not a registered asset: {}", uuid.str());
            return "";
        }

        return asset_registry->GetAssetTypeID(uuid);
    }

    void Exit()
    {
        resources.clear();
        asset_registry.reset();
    }

    void Update()
    {
        std::erase_if(resources, [](const std::pair<const UUID, ResourceRefCount>& element) {
            return element.second.reference_count == 0;
        });
    }

} // namespace ResourceManager