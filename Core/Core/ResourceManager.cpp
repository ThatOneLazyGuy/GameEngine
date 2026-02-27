#include "ResourceManager.hpp"

#include "Tools/Files.hpp"

namespace ResourceManager
{

    void Init()
    {
        usize read_index = 0;
        const std::vector<uint8> data = Files::ReadBinary("ResourceMapping.bin");
        while (read_index < data.size())
        {
            const usize path_size = *reinterpret_cast<const usize*>(data.data() + read_index);
            read_index += sizeof(usize);

            std::string path{data.data() + read_index, data.data() + read_index + path_size};
            read_index += path_size;

            const usize uuid_size = *reinterpret_cast<const usize*>(data.data() + read_index);
            read_index += sizeof(usize);

            std::span uuid_bytes{data.data() + read_index, data.data() + read_index + uuid_size};
            read_index += uuid_size;

            UUID uuid{uuid_bytes.data()};

            Log::Log("Path: {}, UUID: {}", path, uuid.str());

            permanent_mapping.emplace(std::move(path), std::move(uuid));

        }
    }

    void Exit()
    {
        resources.clear();

        usize write_index = 0;
        std::vector<uint8> data;
        for (const auto& [path, uuid] : permanent_mapping)
        {
            const usize path_size = path.size();
            const usize path_data_size = path_size + sizeof(usize);

            const std::string uuid_bytes = uuid.bytes();
            const usize uuid_size = uuid_bytes.size();
            const usize uuid_data_size = uuid_size + sizeof(usize);

            data.resize(data.size() + path_data_size + uuid_data_size);

            std::memcpy(data.data() + write_index, &path_size, sizeof(usize));
            write_index += sizeof(usize);

            std::memcpy(data.data() + write_index, path.data(), path_size);
            write_index += path_size;

            std::memcpy(data.data() + write_index, &uuid_size, sizeof(usize));
            write_index += sizeof(usize);

            std::memcpy(data.data() + write_index, uuid_bytes.data(), uuid_size);
            write_index += uuid_size;

            Log::Log("Path: {}, UUID: {}", path, uuid.str());
        }

        Files::WriteBinary("ResourceMapping.bin", data);

        permanent_mapping.clear();
    }

    void Update()
    {
        std::erase_if(resources, [](const std::pair<const UUID, std::pair<usize, std::unique_ptr<ResourceBase>>>& element) {
            return element.second.first == 0;
        });
    }

} // namespace ResourceManager