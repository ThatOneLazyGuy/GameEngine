#include "RenderPassInterface.hpp"

#include "Core/Physics/Physics.hpp"
#include "Core/Physics/DebugRenderer.hpp"

namespace
{
    void RenderMesh(const Transform& transform, const Handle<Mesh>& mesh_handle)
    {
        Renderer::SetUniform(0, transform.GetMatrix());

        uint32 diffuse_count = 0;
        uint32 specular_count = 0;
        for (const auto& texture : mesh_handle->textures)
        {
            uint32 sampler_slot = 0;
            if (texture->GetFlags() | Texture::DIFFUSE) { sampler_slot = diffuse_count++; }
            else if (texture->GetFlags() | Texture::SPECULAR) { sampler_slot = 3 + specular_count++; }

            Renderer::Instance().SetTextureSampler(sampler_slot, *texture);
        }

        Renderer::Instance().RenderMesh(*mesh_handle);
    }
} // namespace


void DefaultRenderPass::Render()
{
    const ECS::Entity camera_entity = ECS::GetWorld().query_builder<const Transform, const Camera>().build().first();
    const Transform& camera_transform = camera_entity.GetComponent<Transform>();
    const Camera& camera = camera_entity.GetComponent<Camera>();

    const Matrix4 view = Math::Inverse(camera_transform.GetMatrix());
    Renderer::SetUniform(1, view);

    const Matrix4 projection = camera.GetProjection(*render_target);
    Renderer::SetUniform(2, projection);

    const auto mesh_query = ECS::GetWorld().query_builder<const Transform, const Handle<Mesh>>().build();
    mesh_query.each([](const Transform& transform, const Handle<Mesh>& mesh_handle) { RenderMesh(transform, mesh_handle); });

    for (const auto& [model, mesh] : Physics::RenderDebug(camera_transform.GetPosition()))
    {
        Renderer::SetUniform(0, model);

        uint32 diffuse_count = 0;
        uint32 specular_count = 0;
        for (const auto& texture : mesh->textures)
        {
            uint32 sampler_slot = 0;
            if (texture->GetFlags() & Texture::DIFFUSE) sampler_slot = diffuse_count++;
            else if (texture->GetFlags() & Texture::SPECULAR) sampler_slot = 3 + specular_count++;

            Renderer::Instance().SetTextureSampler(sampler_slot, *texture);
        }

        Renderer::Instance().RenderMesh(*mesh);
    }
}