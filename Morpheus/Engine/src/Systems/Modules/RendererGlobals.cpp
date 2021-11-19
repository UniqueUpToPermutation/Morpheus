#include <Engine/Systems/Renderer.hpp>

namespace Morpheus {

    std::unique_ptr<Task> RendererGlobalsModule::Startup(Renderer* renderer) {
        mRenderer = renderer;

        auto func = FunctionPrototype<>([&globals = mGlobals, renderer]() {
            globals.Initialize(renderer->Graphics().Device());
        });

        CustomTask task;
        task.Add(func());
        return std::make_unique<CustomTask>(std::move(task));
    }

    std::unique_ptr<Task> RendererGlobalsModule::LoadResources(Frame* frame) {
        return nullptr;
    }

    void RendererGlobalsModule::NewFrame(Frame* frame) {
    }

    TaskNode RendererGlobalsModule::GenerateTaskNode(Future<RenderParams> future) {
        // Write renderer globals to globals buffer
        auto func = FunctionPrototype<Future<RenderParams>>(
            [&globals = mGlobals, renderer = mRenderer]
            (const TaskParams& params, Future<RenderParams> rendererParams) {
            auto& graphics = renderer->Graphics();
            globals.Write(graphics.ImmediateContext(), rendererParams.Get().mGlobals);
        });
    }

    bool RendererGlobalsModule::AllowMultithreading() const {
        return false;
    }

    DynamicUniformBuffer<RendererGlobals>& RendererGlobalsModule::GetBuffer() {
        return mGlobals;
    }
}