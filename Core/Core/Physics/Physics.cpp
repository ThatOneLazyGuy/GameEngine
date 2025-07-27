#include "Physics.hpp"

#include "DebugRenderer.hpp"
#include "Core/ECS.hpp"

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <cstdarg>
#include <iostream>
#include <thread>

template <typename>
extern void SetUniform(int binding, const Matrix4& value);

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* inFMT, ...)
{
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    std::cout << buffer << '\n';
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint32 inLine)
{
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << '\n';

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS

namespace Layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        switch (a)
        {
        case Layers::NON_MOVING:
            return b == Layers::MOVING; // Non moving only collides with moving
        case Layers::MOVING:
            return true; // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING{0};
    static constexpr JPH::BroadPhaseLayer MOVING{1};
    static constexpr uint32 NUM_LAYERS{2};
}; // namespace BroadPhaseLayers

class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
  public:
    BroadPhaseLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        object_to_broad_phase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        object_to_broad_phase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    uint32 GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        JPH_ASSERT(layer < Layers::NUM_LAYERS);
        return object_to_broad_phase[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(layer))
        {
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):
            return "NON_MOVING";
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):
            return "MOVING";
        default:
            JPH_ASSERT(false);
            return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

  private:
    JPH::BroadPhaseLayer object_to_broad_phase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer a, JPH::BroadPhaseLayer b) const override
    {
        switch (a)
        {
        case Layers::NON_MOVING:
            return b == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

// An example contact listener
class ContactListenerImpl final : public JPH::ContactListener
{
  public:
    // See: ContactListener
    JPH::ValidateResult OnContactValidate(
        const JPH::Body& a, const JPH::Body& b, JPH::RVec3Arg base_offset, const JPH::CollideShapeResult& collision_result
    ) override
    {
        std::cout << "Contact validate callback" << '\n';

        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(
        const JPH::Body& a, const JPH::Body& b, const JPH::ContactManifold& manifold, JPH::ContactSettings& io_settings
    ) override
    {
        std::cout << "A contact was added" << '\n';
    }

    void OnContactPersisted(
        const JPH::Body& a, const JPH::Body& b, const JPH::ContactManifold& manifold, JPH::ContactSettings& io_settings
    ) override
    {
        std::cout << "A contact was persisted" << '\n';
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& sub_shape_pair) override { std::cout << "A contact was removed" << '\n'; }
};

// An example activation listener
class BodyActivationListenerImpl final : public JPH::BodyActivationListener
{
  public:
    void OnBodyActivated(const JPH::BodyID& inBodyID, uint64 inBodyUserData) override { std::cout << "A body got activated" << '\n'; }

    void OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64 inBodyUserData) override { std::cout << "A body went to sleep" << '\n'; }
};

namespace Physics
{
    constexpr uint32 MAX_BODIES = 1024;
    constexpr uint32 NUM_BODY_MUTEXES = 0;
    constexpr uint32 MAX_BODY_PAIRS = 1024;
    constexpr uint32 MAX_CONTACT_CONSTRAINTS = 1024;

    constexpr float DELTA_TIME = 1.0f / 60.0f;
    constexpr int COLLISION_STEPS = 1;

    JPH::TempAllocatorImpl* temp_allocator;
    JPH::JobSystemThreadPool* job_system;

    BroadPhaseLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;

    JPH::PhysicsSystem physics_system;

    BodyActivationListenerImpl body_activation_listener;
    ContactListenerImpl contact_listener;

    JPH::Body* floor;

    void Collider::SetMotionType(const MotionType type)
    {
        motion_type = type;
        physics_system.GetBodyInterface().SetMotionType(body_id, static_cast<JPH::EMotionType>(motion_type), JPH::EActivation::Activate);
    }

    SphereCollider::SphereCollider(const float radius) : radius{radius}
    {
        JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

        const auto* shape = new JPH::SphereShape{radius};
        const JPH::BodyCreationSettings sphere_settings{
            shape, JPH::RVec3{0.0f, 0.0f, 0.0f},
             JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING
        };

        body_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    }

    SphereCollider::SphereCollider() : SphereCollider{1.0f} {}

    SphereCollider::~SphereCollider()
    {
        JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

        body_interface.RemoveBody(body_id);
        body_interface.DestroyBody(body_id);
    }

    void Test(const Collider& collider)
    {
        JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

        const JPH::Vec3 velocity = body_interface.GetLinearVelocity(collider.GetBodyID());
        body_interface.SetLinearVelocity(collider.GetBodyID(), velocity + JPH::Vec3{0.0f, 2.0f, 0.0f});
    }

    DebugRenderer* debug_renderer;

    void Init()
    {
        // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
        // This needs to be done before any other Jolt function is called.
        JPH::RegisterDefaultAllocator();

        // Install trace and assert callbacks
        JPH::Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

        // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
        // It is not directly used in this example but still required.
        JPH::Factory::sInstance = new JPH::Factory();

        // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
        // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
        // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
        JPH::RegisterTypes();

        temp_allocator = new JPH::TempAllocatorImpl{10 * 1024 * 1024};
        job_system = new JPH::JobSystemThreadPool{
            JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, static_cast<int>(std::thread::hardware_concurrency() - 1)
        };

        physics_system.Init(
            MAX_BODIES,
            NUM_BODY_MUTEXES,
            MAX_BODY_PAIRS,
            MAX_CONTACT_CONSTRAINTS,
            broad_phase_layer_interface,
            object_vs_broadphase_layer_filter,
            object_vs_object_layer_filter
        );

        physics_system.SetBodyActivationListener(&body_activation_listener);
        physics_system.SetContactListener(&contact_listener);

        JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

        // Next we can create a rigid body to serve as the floor, we make a large box
        // Create the settings for the collision volume (the shape).
        // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
        const JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3{100.0f, 1.0f, 100.0f});
        floor_shape_settings.SetEmbedded();
        // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

        // Create the shape
        const JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
        const JPH::ShapeRefC floor_shape = floor_shape_result.Get();
        // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

        // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
        const JPH::BodyCreationSettings floor_settings(
            floor_shape, JPH::RVec3(0.0f, -2.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING
        );

        // Create the actual rigid body
        floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

        // Add it to the world
        body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);

        physics_system.OptimizeBroadPhase();

        debug_renderer = new DebugRenderer();
    }

    void Update(const float delta_time)
    {
        JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

        auto query = ECS::GetWorld().query_builder<Transform, const SphereCollider>().build();

        query.each([&body_interface](Transform& transform, const SphereCollider& collider) {
            JPH::RVec3 position;
            JPH::Quat rotation;

            body_interface.GetPositionAndRotation(collider.GetBodyID(), position, rotation);

            transform.SetPosition(float3{position.mF32});
            transform.SetRotation(Quat{rotation.mValue.mF32});
        });

        physics_system.Update(delta_time, COLLISION_STEPS, temp_allocator, job_system);
    }

    void Exit()
    {
        delete debug_renderer;

        JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

        // Remove and destroy the floor
        body_interface.RemoveBody(floor->GetID());
        body_interface.DestroyBody(floor->GetID());

        // Unregisters all types with the factory and cleans up the default material
        JPH::UnregisterTypes();

        // Destroy the factory
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        delete temp_allocator;
        temp_allocator = nullptr;

        delete job_system;
        job_system = nullptr;
    }

    std::vector<RenderData> RenderDebug(const float3& camera_position)
    {
        static constexpr JPH::BodyManager::DrawSettings draw_settings{};
        debug_renderer->mCameraPos = JPH::RVec3{camera_position.x(), camera_position.y(), camera_position.z()};

        physics_system.DrawBodies(draw_settings, debug_renderer);

        debug_renderer->NextFrame();

        std::vector<RenderData> render_data{std::move(debug_renderer->render_data)};
        debug_renderer->render_data.clear();

        return std::move(render_data);
    }
} // namespace Physics
