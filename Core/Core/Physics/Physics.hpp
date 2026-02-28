#pragma once

#include "Core/Math.hpp"
#include "Tools/uuid.hpp"
#include "Tools/Types.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>


namespace Physics
{
    struct RenderData;

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

        Collider(Collider&& other) noexcept : body_id{other.body_id}, motion_type{other.motion_type}
        {
            other.body_id = BodyID{JPH::BodyID::cInvalidBodyID};
        }

        [[nodiscard]] virtual Type GetType() const = 0;

        virtual void SetMotionType(MotionType type);
        [[nodiscard]] MotionType GetMotionType() const { return motion_type; }

        [[nodiscard]] BodyID GetBodyID() const { return body_id; }

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

        [[nodiscard]] float GetRadius() const { return radius; }
        void SetRadius(float new_radius);

      private:
        float radius{1.0f};
    };

    void Init(const UUID& shader_uuid);

    void Update(float delta_time);

    void Exit();

    class DebugRenderer;
    inline DebugRenderer* debug_renderer = nullptr;
} // namespace Physics