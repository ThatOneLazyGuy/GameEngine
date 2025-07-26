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
            JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow
        ) override
        {
            DrawLine(inV1, inV2, inColor);
            DrawLine(inV2, inV3, inColor);
            DrawLine(inV3, inV1, inColor);
        }

        Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;
        Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const uint32* inIndices, int inIndexCount) override;
        void DrawGeometry(
            JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor,
            const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode
        ) override;

        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

        void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;

        JPH::RVec3 mCameraPos;
        std::vector<RenderData> render_data;

      private:
        class BatchImpl : public JPH::RefTargetVirtual
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