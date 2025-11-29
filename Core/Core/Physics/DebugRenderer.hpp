#pragma once

#include "Core/Rendering/RenderPassInterface.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

namespace Physics
{
    struct RenderData
    {
        Matrix4 model_matrix;
        const Mesh* mesh;
    };

    class DebugRenderer final : public JPH::DebugRenderer
    {
      public:
        DebugRenderer();

        void DrawTriangle(JPH::RVec3Arg vertex1, JPH::RVec3Arg vertex2, JPH::RVec3Arg vertex3, JPH::ColorArg color, ECastShadow cast_shadow)
            override
        {
            DrawLine(vertex1, vertex2, color);
            DrawLine(vertex2, vertex3, color);
            DrawLine(vertex3, vertex1, color);
        }

        Batch CreateTriangleBatch(const Triangle* triangles, int triangle_count) override;
        Batch CreateTriangleBatch(const Vertex* vertices, int vertices_count, const uint32* indices, int indices_count) override;
        void DrawGeometry(
            JPH::RMat44Arg model_matrix, const JPH::AABox& world_space_bounds, float lod_scale_sq, JPH::ColorArg model_color,
            const GeometryRef& geometry, ECullMode cull_mode, ECastShadow cast_shadow, EDrawMode draw_mode
        ) override;

        void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) override {}
        void DrawText3D(JPH::RVec3Arg position, const std::string_view& string, JPH::ColorArg color, float height) override {}

        JPH::RVec3 camera_position;
        std::vector<RenderData> render_data;

      private:
        class BatchMesh final : public JPH::RefTargetVirtual
        {
          public:
            JPH_OVERRIDE_NEW_DELETE

            BatchMesh(const std::vector<::Vertex>& vertices, const std::vector<uint32>& indices) : mesh{vertices, indices} {}

            void AddRef() override { ++reference_count; }
            void Release() override
            {
                if (--reference_count == 0) delete this;
            }

            Mesh mesh;

          private:
            std::atomic<uint32> reference_count{0};
        };
    };

    class PhysicsDebugRenderPass : public RenderPassInterface
    {
      public:
        PhysicsDebugRenderPass(const Handle<GraphicsShaderPipeline>& pipeline, const Handle<RenderTarget>& target) :
            RenderPassInterface{pipeline, target}
        {
        }
        ~PhysicsDebugRenderPass() override = default;

        void Render() override;
    };

} // namespace Physics