#pragma once

#include <string_view>

#ifdef _MSC_VER
template <typename Type> constexpr std::string_view RawName()
{
    return std::string_view{__FUNCSIG__ + 82, sizeof(__FUNCSIG__) - 90};
}
constexpr std::string_view anonymous_namespace{"`anonymous-namespace'"};
#elif __clang__
template <typename Type> constexpr std::string_view RawName()
{
    return std::string_view{__PRETTY_FUNCTION__ + 35, sizeof(__PRETTY_FUNCTION__) - 37};
}
constexpr std::string_view anonymous_namespace{"(anonymous namespace)"};
#elif __GNUC__
template <typename Type> constexpr std::string_view RawName()
{
    return std::string_view{__PRETTY_FUNCTION__ + 50, sizeof(__PRETTY_FUNCTION__) - 101};
}
constexpr std::string_view anonymous_namespace{"{anonymous}"};
#else
    #error TypeNames not supported on this compiler!
#endif

template <typename E> std::string_view GetName()
{
    constexpr std::string_view raw_name = RawName<E>();

    constexpr size_t anonymous_namespace_pos = raw_name.find(anonymous_namespace);
    if (anonymous_namespace_pos == std::string_view::npos)
    {
        constexpr size_t space_pos = raw_name.find_first_of(' ');
        if (space_pos == std::string_view::npos) return raw_name;

        constexpr size_t start = space_pos + 1;
        return std::string_view{raw_name.data() + start, raw_name.size() - start};
    }

    constexpr size_t start = anonymous_namespace_pos + anonymous_namespace.size() + 2;
    return std::string_view{raw_name.data() + start, raw_name.size() - start};
}