#include "DebugRenderer.hpp"

#include "Physics.hpp"
#include "Core/Rendering/Renderer.hpp"

#include <numeric>

namespace Physics
{
    DebugRenderer::DebugRenderer() { Initialize(); }

    JPH::DebugRenderer::Batch DebugRenderer::CreateTriangleBatch(const Triangle* triangles, int triangle_count)
    {
        BatchMesh* batch = new BatchMesh;

        for (int triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
        {
            for (auto [position, normal, ui, color] : triangles[triangle_index].mV)
            {
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
        BatchMesh* batch = new BatchMesh{};

        batch->mesh.vertices.resize(vertices_count);
        for (int i = 0; i < vertices_count; i++)
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
        const LOD* lod = &geometry->GetLOD(JPH::Vec3{camera_position}, world_space_bounds, lod_scale_sq);
        const BatchMesh* batch = dynamic_cast<const BatchMesh*>(lod->mTriangleBatch.GetPtr());

        Matrix4 model{};
        std::memcpy(&model, &model_matrix, sizeof(model));

        render_data.emplace_back(model, &batch->mesh);
    }

    void PhysicsDebugRenderPass::Render()
    {
        const ECS::Entity camera_entity = ECS::GetWorld().query_builder<const Transform, const Camera>().build().first();
        const Transform& camera_transform = camera_entity.GetComponent<Transform>();
        const Camera& camera = camera_entity.GetComponent<Camera>();

        const float3& camera_position = camera_transform.GetPosition();
        debug_renderer->camera_position = JPH::RVec3{camera_position.x(), camera_position.y(), camera_position.z()};
        debug_renderer->NextFrame();

        const Matrix4 view = Math::Inverse(camera_transform.GetMatrix());
        Renderer::SetUniform(1, view);

        const Matrix4 projection = camera.GetProjection(*render_target);
        Renderer::SetUniform(2, projection);

        for (const auto& [model, mesh] : debug_renderer->render_data)
        {
            Renderer::SetUniform(0, model);
            Renderer::Instance().RenderMesh(*mesh);
        }

        debug_renderer->render_data.clear();
    }
} // namespace Physics