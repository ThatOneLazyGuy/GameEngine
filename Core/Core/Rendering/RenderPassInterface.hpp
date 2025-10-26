#pragma once

#include "Renderer.hpp"

class RenderPassInterface
{
  public:
    RenderPassInterface(const Handle<GraphicsShaderPipeline>& pipeline, const Handle<RenderTarget>& target) :
        graphics_pipeline{pipeline}, render_target{target}
    {
    }
    virtual ~RenderPassInterface() = default;

    Handle<GraphicsShaderPipeline> graphics_pipeline;
    Handle<RenderTarget> render_target;

    virtual void Render() = 0;
};

class DefaultRenderPass final : public RenderPassInterface
{
  public:
    DefaultRenderPass(const Handle<GraphicsShaderPipeline>& pipeline, const Handle<RenderTarget>& target) :
        RenderPassInterface{pipeline, target}
    {
    }
    ~DefaultRenderPass() override = default;

    void Render() override;
};