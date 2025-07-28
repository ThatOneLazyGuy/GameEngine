#include "DebugRenderer.hpp"

#include "Physics.hpp"
#include "Core/Renderer.hpp"

namespace Physics
{
    DebugRenderer::DebugRenderer() { Initialize(); }

    JPH::DebugRenderer::Batch DebugRenderer::CreateTriangleBatch(const Triangle* triangles, int triangle_count)
    {
        BatchMesh* batch = new BatchMesh;

        for (size triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
        {
            for (size i = 0; i < 3; i++)
            {
                auto& [position, normal, ui, color] = triangles[triangle_index].mV[i];

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
        const Vertex* vertices, const int vertices_count, const uint32* indices, const int inIndexCount
    )
    {
        BatchMesh* batch = new BatchMesh;

        batch->mesh.vertices.resize(vertices_count);
        for (size i = 0; i < vertices_count; i++)
        {
            auto& [position, normal, ui, color] = vertices[i];

            auto& vertex = batch->mesh.vertices[i];

            vertex.position = float3{position.x, position.y, position.z};
            const JPH::Vec4 float_color = color.ToVec4();
            vertex.color = float3{float_color.mF32};
        }

        batch->mesh.indices.assign(indices, indices + inIndexCount);

        Renderer::Instance().CreateMesh(batch->mesh);
        return batch;
    }


    void DebugRenderer::DrawGeometry(
        JPH::RMat44Arg model_matrix, const JPH::AABox& world_space_bounds, float lod_scale_sq, JPH::ColorArg model_color,
        const GeometryRef& geometry, ECullMode cull_mode, ECastShadow cast_shadow, EDrawMode draw_mode
    )
    {
        const LOD* lod = &geometry->GetLOD(JPH::Vec3{mCameraPos}, world_space_bounds, lod_scale_sq);
        const BatchMesh* batch = static_cast<const BatchMesh*>(lod->mTriangleBatch.GetPtr());

        Matrix4 model;
        std::memcpy(&model, &model_matrix, sizeof(model));

        render_data.emplace_back(model, &batch->mesh);
    }
}