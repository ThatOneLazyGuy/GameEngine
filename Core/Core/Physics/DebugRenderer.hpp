#pragma once

#include "Core/Renderer.hpp"

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

        void DrawTriangle(
            JPH::RVec3Arg vertex1, JPH::RVec3Arg vertex2, JPH::RVec3Arg vertex3, JPH::ColorArg color, ECastShadow cast_shadow
        ) override
        {
            DrawLine(vertex1, vertex2, color);
            DrawLine(vertex2, vertex3, color);
            DrawLine(vertex3, vertex1, color);
        }

        Batch CreateTriangleBatch(const Triangle* triangles, int triangle_count) override;
        Batch CreateTriangleBatch(const Vertex* vertices, int vertices_count, const uint32* indices, int inIndexCount) override;
        void DrawGeometry(
            JPH::RMat44Arg model_matrix, const JPH::AABox& world_space_bounds, float lod_scale_sq, JPH::ColorArg model_color,
            const GeometryRef& geometry, ECullMode cull_mode, ECastShadow cast_shadow, EDrawMode draw_mode
        ) override;

        void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) override {};
        void DrawText3D(JPH::RVec3Arg position, const std::string_view& string, JPH::ColorArg color, float height) override {};

        JPH::RVec3 mCameraPos;
        std::vector<RenderData> render_data;

      private:
        class BatchMesh final : public JPH::RefTargetVirtual
        {
          public:
            JPH_OVERRIDE_NEW_DELETE

            virtual void AddRef() override { ++mRefCount; }
            virtual void Release() override
            {
                if (--mRefCount == 0) delete this;
            }

            Mesh mesh;

          private:
            std::atomic<uint32> mRefCount = 0;
        };
    };
} // namespace Physics