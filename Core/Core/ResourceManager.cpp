#include "ResourceManager.hpp"

namespace ResourceManager
{

    void Exit() 
    {
        resources.clear();
    }

    void Update()
    {
        std::erase_if(resources, [](const std::pair<const usize, std::pair<usize, std::unique_ptr<ResourceBase>>>& element) {
            return element.second.first == 0;
        });
    }

} // namespace ResourceManager