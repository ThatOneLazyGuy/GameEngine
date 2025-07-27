#pragma once

#include "Math.hpp"
#include <flecs.h>

class Transform
{
  public:
    Transform() = default;

    const Matrix4& GetMatrix()
    {
        matrix = Math::Identity<Matrix4>();

        matrix *= Math::Scale(scale);
        matrix *= Math::Rotation(rotation);
        matrix *= Math::Translation(position);

        return matrix;
    }

    float3 position{0.0f, 0.0f, 0.0f};
    Quat rotation{Math::Identity<Quat>()};
    float3 scale{1.0f, 1.0f, 1.0f};

  private:
    Matrix4 matrix{Math::Identity<Matrix4>()};
};

namespace ECS
{
    using Entity = flecs::entity;

    void Init();

    void Exit();

    [[nodiscard]] flecs::world& GetWorld();
} // namespace ECS