#pragma once

#include "Math.hpp"
#include <flecs.h>

class Transform
{
  public:
    Transform() = default;

    const Mat4& GetMatrix()
    {
        matrix = Math::Identity<Mat4>();

        matrix *= Math::Scale(scale);
        matrix *= Math::Rotation(rotation);
        matrix *= Math::Translation(position);

        return matrix;
    }

    Vec3 position{0.0f, 0.0f, 0.0f};
    Quat rotation = Math::Identity<Quat>();
    Vec3 scale{1.0f, 1.0f, 1.0f};

  private:
    Mat4 matrix = Math::Identity<Mat4>();
};

namespace ECS
{
    using Entity = flecs::entity;

    inline flecs::world& GetWorld()
    {
        static flecs::world world;
        return world;
    }
} // namespace ECS