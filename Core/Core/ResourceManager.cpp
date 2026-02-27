#include "ResourceManager.hpp"

#include "Tools/Files.hpp"

namespace ResourceManager
{

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