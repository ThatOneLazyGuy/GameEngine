#pragma once

#include <vector>

template <typename Type, typename HasherType = std::hash<Type>>
class UniqueVector
{
  public:
    [[nodiscard]] auto Begin() { return data.begin(); }
    [[nodiscard]] auto End() { return data.end(); }
    [[nodiscard]] auto Begin() const { return data.begin(); }
    [[nodiscard]] auto End() const { return data.end(); }
    [[nodiscard]] auto CBegin() const { return data.cbegin(); }
    [[nodiscard]] auto CEnd() const { return data.cend(); }

    [[nodiscard]] bool Empty() const { return data.empty(); }
    [[nodiscard]] usize Size() const { return data.size(); }

    void Clear()
    {
        data.clear();
        mapping.clear();
    }

    void PushBack(const Type& value)
    {
        const usize index = data.size();
        if (AddMapping(value, index)) return;

        data.push_back(value);
    }
    template <typename Iterator>
    void PushBack(const Iterator& begin, const Iterator& end)
    {
        data.reserve(data.size() + std::distance(begin, end));

        Iterator iterator = begin;
        while (iterator != end)
        {
            PushBack(*iterator);
            ++iterator;
        }
    }
    template <typename Container>
    void PushBack(const Container& container)
    {
        PushBack(std::begin(container), std::end(container));
    }
    void PushBack(Type&& value)
    {
        const usize index = data.size();
        if (AddMapping(value, index)) return;

        data.push_back(std::move(value));
    }

    template <typename... Args>
    void EmplaceBack(Args&&... args)
    {
        Type value{std::forward<Args>(args)...};

        PushBack(std::move(value));
    }

    void Erase(const Type& value)
    {
        const auto&& [hash, mapping_iterator] = GetNextMappingIterator(value);

        data.erase(data.begin() + mapping_iterator->value_index);

        // Decrement the index value of all the elements in the mapping after the erased one.
        for (auto iterator = mapping_iterator; iterator != mapping.end(); ++iterator)
        {
            --iterator->value_index;
        }

        mapping.erase(mapping_iterator);
    }
    void EraseIndex(const typename std::vector<Type>::const_iterator& position)
    {
        const usize i = std::distance(data.begin(), position);

        const auto mapping_iterator =
            std::ranges::find(mapping, [i](const std::pair<usize, usize>& element) { return i == element.second; });

        data.erase(position);

        for (auto iterator = mapping_iterator; iterator != data.end(); ++iterator)
        {
            --iterator->value_index;
        }

        mapping.erase(mapping_iterator);
    }

    [[nodiscard]] Type& Front() { return data.front(); }
    [[nodiscard]] Type& Back() { return data.back(); }
    [[nodiscard]] const Type& Front() const { return data.front(); }
    [[nodiscard]] const Type& Back() const { return data.back(); }

    [[nodiscard]] Type* Find(const Type& value)
    {
        const auto&& [hash, mapping_iterator] = GetNextMappingIterator(value);

        if (mapping_iterator == mapping.end() || hash != mapping_iterator->value_hash) return nullptr;

        return data[mapping_iterator->value_index];
    }

    [[nodiscard]] const Type* Find(const Type& value) const
    {
        const auto&& [hash, mapping_iterator] = GetNextMappingIterator(value);

        if (mapping_iterator == mapping.end() || hash != mapping_iterator->value_hash) return nullptr;

        return data[mapping_iterator->value_index];
    }

    [[nodiscard]] bool Contains(const Type& value)
    {
        const auto&& [hash, mapping_iterator] = GetNextMappingIterator(value);

        return mapping_iterator != mapping.end() && hash == mapping_iterator->value_hash;
    }

    Type& operator[](usize i) { return data[i].data; }
    const Type& operator[](usize i) const { return data[i].data; }

  private:
    class ValueMapping
    {
      public:
        // Lesser than operator is used in std::ranges::lower_bounds() to do a binary search on the hashes in the "mapping" vector.
        [[nodiscard]] bool operator<(const ValueMapping& other) const { return value_hash < other.value_hash; }

        usize value_hash;
        usize value_index;
    };

    // Hashes the value and does a binary search to find its position, or the position it *should* be in the "mapping" vector.
    std::pair<usize, typename std::vector<ValueMapping>::iterator> GetNextMappingIterator(const Type& value)
    {
        const usize hash = HasherType{}(value);
        return std::pair<usize, typename decltype(mapping)::iterator>{
            hash, std::ranges::lower_bound(mapping, ValueMapping{hash, hash}, std::less{})
        };
    }

    // Returns true if the value is already part of the mapping.
    [[nodiscard]] bool AddMapping(const Type& value, const usize data_index)
    {
        const auto&& [hash, mapping_iterator] = GetNextMappingIterator(value);
        if (mapping_iterator != mapping.end() && hash == mapping_iterator->value_hash) return true;

        mapping.emplace(mapping_iterator, ValueMapping{hash, data_index});
        return false;
    }

    std::vector<Type> data;
    std::vector<ValueMapping> mapping; // Mapping between value hash and index in data vector.
};