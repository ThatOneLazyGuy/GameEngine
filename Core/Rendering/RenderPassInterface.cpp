#include "RenderPassInterface.hpp"

#include "Transform.hpp"

namespace
{
    void RenderMesh(const ECS::Entity& entity, const ResourceRef<Mesh>& mesh_handle)
    {
        Renderer::SetUniform(0, Transform::GetMatrix(entity));

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


void DefaultRenderPass::Render(const ECS::Entity& camera_entity)
{
    const Camera& camera = camera_entity.GetComponent<Camera>();

    const Matrix4 view = Math::Inverse(Transform::GetMatrix(camera_entity));
    Renderer::SetUniform(1, view);

    const Matrix4 projection = camera.GetProjection(*render_target);
    Renderer::SetUniform(2, projection);

    const auto mesh_query = ECS::GetWorld().query_builder<const ResourceRef<Mesh>>().without<ECS::IgnoreTag>().build();
    mesh_query.each([](const ECS::Entity& entity, const ResourceRef<Mesh>& mesh_handle) {
        if (!mesh_handle.Valid()) return;

        RenderMesh(entity, mesh_handle);
    });
}