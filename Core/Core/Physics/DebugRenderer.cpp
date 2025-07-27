#include "DebugRenderer.hpp"

#include "Physics.hpp"
#include "Core/Renderer.hpp"

namespace Physics
{
    DebugRenderer::DebugRenderer() { Initialize(); }

    JPH::DebugRenderer::Batch DebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
    {
        BatchImpl* batch = new BatchImpl;

        for (size triangle_index = 0; triangle_index < inTriangleCount; ++triangle_index)
        {
            for (size i = 0; i < 3; i++)
            {
                auto& [position, normal, ui, color] = inTriangles[triangle_index].mV[i];

                auto& vertex = batch->mesh.vertices.emplace_back();

                vertex.position = float3{position.x, position.y, position.z};
                const JPH::Vec4 float_color = color.ToVec4();
                vertex.color = float3{float_color.mF32};
            }
        }

        std::iota(batch->mesh.indices.begin(), batch->mesh.indices.end(), 0);

        Renderer::Instance().CreateMesh(batch->mesh);
        return batch;
    }

    JPH::DebugRenderer::Batch DebugRenderer::CreateTriangleBatch(
        const Vertex* inVertices, int inVertexCount, const uint32* inIndices, int inIndexCount
    )
    {
        BatchImpl* batch = new BatchImpl;

        batch->mesh.vertices.resize(inVertexCount);
        for (size i = 0; i < inVertexCount; i++)
        {
            auto& [position, normal, ui, color] = inVertices[i];

            auto& vertex = batch->mesh.vertices[i];

            vertex.position = float3{position.x, position.y, position.z};
            const JPH::Vec4 float_color = color.ToVec4();
            vertex.color = float3{float_color.mF32};
        }

        batch->mesh.indices.assign(inIndices, inIndices + inIndexCount);

        Renderer::Instance().CreateMesh(batch->mesh);
        return batch;
    }


    void DebugRenderer::DrawGeometry(
        JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor,
        const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode
    )
    {
        const LOD* lod = &inGeometry->GetLOD(JPH::Vec3{mCameraPos}, inWorldSpaceBounds, inLODScaleSq);
        const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());

        Matrix4 model;
        std::memcpy(&model, &inModelMatrix, sizeof(model));

        render_data.emplace_back(model, &batch->mesh);
    }

    void DebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) { }

    void DebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) {}
}