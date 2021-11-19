#include <Engine/Systems/Renderer.hpp>

namespace Morpheus {
    std::unique_ptr<Task> Renderer::Startup(SystemCollection& systems) {

        // Simply pass the startup call to all of the renderer modules
        CustomTask task;

        std::vector<std::shared_ptr<Task>> startups;

        for (auto& mod : mModules) {
            auto startupResult = mod.second->Startup(this);
            if (startupResult) {
                startups.emplace_back(std::shared_ptr<Task>(startupResult.get()));
            }
        }

        auto finalize = FunctionPrototype<>([&initialized = bInitialized](const TaskParams&) {
           initialized = true;
        })();

        for (auto& startup : startups) {
            task.Add(startup);
            finalize.After(startup->OutNode());
        }

        task.Add(finalize);

        return std::make_unique<CustomTask>(std::move(task));
    }

    bool Renderer::IsInitialized() const {
        return bInitialized;
    }

    void Renderer::Shutdown() {
        mModules.clear();
        bInitialized = false;
    }

    IRenderModule* Renderer::AddModule(std::unique_ptr<IRenderModule>&& module) {
        auto type = module->GetType();
        auto it = mModules.find(type.info());

        if (it != mModules.end()) {
            throw std::runtime_error("Module already in place!");
        }

        auto modulePtr = module.get();

        mModules.emplace_hint(it, type, std::move(module));

        return modulePtr;
    }

    std::unique_ptr<Task> Renderer::LoadResources(Frame* frame) {
        CustomTask task;

        std::vector<std::shared_ptr<Task>> startups;

        for (auto& mod : mModules) {
            auto startupResult = mod.second->Startup(this);
            if (startupResult) {
                task.Add(std::shared_ptr<Task>(startupResult.release()));
            }
        }

        return std::make_unique<CustomTask>(std::move(task));
    }

    void Renderer::NewFrame(Frame* frame) {
        for (auto& module : mModules) {
            module.second->NewFrame(frame);
        }
    }

    void Renderer::OnAddedTo(SystemCollection& collection) {
        auto task = Render(collection.GetFrameProcessor().GetRenderInput());

        collection.RegisterInterface<IVertexFormatProvider>(this);
        collection.AddRenderTask(std::shared_ptr<Task>(task.release()));
    }

    std::unique_ptr<Task> Renderer::Render(Future<RenderParams> params) {

        CustomTask task;

        std::unordered_map<IRenderModule*, TaskNode> moduleNodes;

        for (auto& module : mModules) {
            auto node = module.second->GenerateTaskNode(params);
            moduleNodes[module.second.get()] = node;
        }

        // Make sure that all modules are executed in the correct order
        for (auto& module : mModules) {
            auto node = moduleNodes[module.second.get()];

            if (node) {
                for (auto dependency : module.second->mDependencies) {
                    auto depNode = moduleNodes[dependency];

                    node.After(depNode);
                }

                task.Add(node);
            }
        }

        return std::make_unique<CustomTask>(std::move(task));
    }
}