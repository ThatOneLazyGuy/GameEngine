#pragma once

#include "Renderer.hpp"

class RenderPassInterface
{
  public:
    RenderPassInterface(const ResourceRef<GraphicsShaderPipeline>& pipeline, const ResourceRef<RenderTarget>& target) :
        graphics_pipeline{pipeline}, render_target{target}
    {
    }
    virtual ~RenderPassInterface() = default;

    ResourceRef<GraphicsShaderPipeline> graphics_pipeline;
    ResourceRef<RenderTarget> render_target;

    bool clear_render_targets{true};

    virtual void Render(const ECS::Entity& camera_entity) = 0;
};

class DefaultRenderPass final : public RenderPassInterface
{
  public:
    DefaultRenderPass(const ResourceRef<GraphicsShaderPipeline>& pipeline, const ResourceRef<RenderTarget>& target) :
        RenderPassInterface{pipeline, target}
    {
    }
    ~DefaultRenderPass() override = default;

    void Render(const ECS::Entity& camera_entity) override;
};