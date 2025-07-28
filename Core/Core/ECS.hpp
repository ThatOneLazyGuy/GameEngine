#pragma once

#include "Math.hpp"
#include <flecs.h>

class Transform
{
  public:
    Transform() = default;

    const Matrix4& GetMatrix() const
    {
        if (IsDirty())
        {
            matrix = Math::Identity<Matrix4>();

            matrix *= Math::Scale(scale);
            matrix *= Math::Rotation(rotation);
            matrix *= Math::Translation(position);
        }

        return matrix;
    }

    const float3& GetPosition() const { return position; }
    const Quat& GetRotation() const { return rotation; }
    const float3& GetScale() const { return scale; }

    void SetPosition(const float3& pos)
    {
        position = pos;
        SetDirty();
    }

    void SetRotation(const Quat& rot)
    {
        rotation = rot;
        SetDirty();
    }

    void SetScale(const float3& size)
    {
        scale = size;
        SetDirty();
    }

  private:
    void SetDirty() const { matrix(3, 3) = 0.0f; }
    [[nodiscard]] bool IsDirty() const { return matrix(3, 3) == 0.0f; }

    float3 position{0.0f, 0.0f, 0.0f};
    Quat rotation{Math::Identity<Quat>()};
    float3 scale{1.0f, 1.0f, 1.0f};

    mutable Matrix4 matrix{Math::Identity<Matrix4>()};
};

namespace ECS
{
    class Entity : protected flecs::entity
    {
      public:
        Entity() = default;
        Entity(flecs::entity&& entity) : flecs::entity{std::move(entity)} {}

        [[nodiscard]] std::string_view Name() const { return std::string_view{name().c_str(), name().size()}; }

        template <typename Type, typename... Args>
        const Entity& AddComponent(Args&&... args) const
        {
            (void)set<Type>(std::forward<Args>(args)...);
            return *this;
        }

        template <typename Type, typename...>
        Entity& AddComponent()
        {
            (void)add<Type>();
            return *this;
        }

        template <typename Type>
        Type& GetComponent()
        {
            return get_mut<Type>();
        }

        template <typename Type>
        const Type& GetComponent() const
        {
            return get<Type>();
        }
    };

    void Init();

    void Exit();

    Entity CreateEntity(const std::string& string);

    [[nodiscard]] flecs::world& GetWorld();
} // namespace ECS