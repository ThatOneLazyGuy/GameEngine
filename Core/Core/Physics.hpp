#pragma once

#include "Tools/Types.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace Physics
{
    class Collider
    {
      public:
        using BodyID = JPH::BodyID;

        enum class Type : uint8
        {
            BOX,
            SPHERE,
            POLYGON
        };

        enum class MotionType : uint8
        {
            KINEMATIC,
            STATIC,
            DYNAMIC,
        };

        Collider() = default;
        virtual ~Collider() = default;

        virtual Type GetType() const = 0;

        virtual void SetMotionType(MotionType type);
        MotionType GetMotionType() const { return motion_type; }

        BodyID GetBodyID() const { return body_id; }

      protected:
        BodyID body_id{JPH::BodyID::cInvalidBodyID};
        MotionType motion_type{MotionType::DYNAMIC};
    };

    class BoxCollider final : public Collider
    {
      public:
        BoxCollider() = default;
        ~BoxCollider() override = default;

        [[nodiscard]] Type GetType() const override { return Type::BOX; }
    };

    class SphereCollider final : public Collider
    {
      public:
        SphereCollider();
        explicit SphereCollider(float radius);
        ~SphereCollider() override;

        [[nodiscard]] Type GetType() const override { return Type::SPHERE; }

        float GetRadius() const { return radius; }
        void SetRadius(const float new_radius) { radius = new_radius; }

      private:
        float radius{1.0f};
    };

    void Test(const Collider& collider);

    void Init();

    void Update(float delta_time);

    void Exit();

} // namespace Physics