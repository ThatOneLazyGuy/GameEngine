#pragma once

#include "Core/ECS.hpp"
#include "Core/Math.hpp"

class Transform
{
  public:
    Transform() = default;

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

    static Matrix4 GetMatrix(const ECS::Entity& entity)
    {
        auto parent_matrix = Math::Identity<Matrix4>();

        const ECS::Entity& parent = entity.GetParent();
        if (parent.Valid()) parent_matrix = GetMatrix(parent);

        const Transform& transform = entity.GetComponent<Transform>();
        if (transform.IsDirty())
        {
            transform.RecalculateMatrix();
            transform.matrix *= parent_matrix;

            for (const ECS::Entity& child : entity.GetChildren())
            {
                child.GetComponent<Transform>().SetDirty();
            }
        }

        return transform.matrix;
    }

  private:
    void SetDirty() const { matrix(3, 3) = 0.0f; }
    [[nodiscard]] bool IsDirty() const { return matrix(3, 3) == 0.0f; }

    void RecalculateMatrix() const
    {
        matrix = Math::Identity<Matrix4>();

        matrix *= Math::Scale(scale);
        matrix *= Math::Rotation(rotation);
        matrix *= Math::Translation(position);
    }

    float3 position{0.0f, 0.0f, 0.0f};
    Quat rotation{Math::Identity<Quat>()};
    float3 scale{1.0f, 1.0f, 1.0f};

    mutable Matrix4 matrix{Math::Identity<Matrix4>()};
};