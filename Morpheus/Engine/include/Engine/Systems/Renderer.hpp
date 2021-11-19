#pragma once

#include <Engine/Systems/System.hpp>
#include <Engine/GeometryStructures.hpp>
#include <Engine/Buffer.hpp>

#include <shaders/StaticMesh/TexturedStaticMesh.hlsl>
#include <shaders/Utils/BasicStructures.hlsl>

namespace Morpheus {
    class Renderer;

    enum class RenderViewType {
        VIEW_TYPE_NORMAL,
        VIEW_TYPE_SHADOW_MAP
    };

    struct RenderView {
        RenderViewType mType;
        entt::entity mCamera;
        int mViewId;
    };

    class IStaticMeshVertexFormatProvider {
        virtual VertexLayout GetStaticMeshLayout() const = 0;
    };

    class IRenderModule {
    public:
        virtual std::unique_ptr<Task> Startup(Renderer* renderer) = 0;
        // Load the necessary resources to render this frame
        virtual std::unique_ptr<Task> LoadResources(Frame* frame) = 0;

        virtual void NewFrame(Frame* frame) = 0;
        virtual TaskNode GenerateTaskNode(
            Future<RenderParams> params,
            Future<RenderView> viewParams) = 0;

        // If this is false, module must be executed on main thread!
        virtual bool AllowMultithreading() const = 0;

        // If this is true, module cannot be executed on main thread!
        virtual bool IsLoadingModule() const = 0;
        virtual entt::meta_type GetType() const = 0;

        virtual ~IRenderModule() = default;

        friend class Renderer;
    };
    
    class Renderer : 
        public ISystem,
		public IVertexFormatProvider {
    private:
        std::unordered_map<entt::type_info, 
			std::unique_ptr<IRenderModule>, 
            TypeInfoHasher> mModules;
            
        RealtimeGraphics& mGraphics;
        std::atomic<bool> bInitialized = false;

        IStaticMeshVertexFormatProvider* mStaticMeshFormatProvider = nullptr;

        DynamicUniformBuffer<HLSL::ViewAttribs> mViewAttribBuffer;
        std::vector<std::unique_ptr<Task>> mRenderViewTasks;

        constexpr static int mMaxViewCount = 16;

    public:
        IStaticMeshVertexFormatProvider* SetStaticMeshFormatProvider(IStaticMeshVertexFormatProvider* provider) {
            auto last = mStaticMeshFormatProvider;
            mStaticMeshFormatProvider = provider;
            return last;
        }

        inline Renderer(RealtimeGraphics& graphics) : 
            mGraphics(graphics) {
        }

        inline RealtimeGraphics& Graphics() {
            return mGraphics;
        }

        IRenderModule* AddModule(std::unique_ptr<IRenderModule>&& module) {
            auto type = module->GetType();
            auto it = mModules.find(type.info());

            if (it != mModules.end()) {
                throw std::runtime_error("Module already in place!");
            }

            auto modulePtr = module.get();

            mModules.emplace_hint(it, type, std::move(module));

            return modulePtr;
        }

        template <typename T, typename ... Args>
		IRenderModule* MakeModule(Args&& ... args) {
            auto instance = std::make_unique<T>(std::forward(args...));
            return AddModule(std::move(instance));
        }

        std::unique_ptr<Task> Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		std::unique_ptr<Task> LoadResources(Frame* frame) override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;

        DG::TEXTURE_FORMAT GetIntermediateFramebufferFormat() const;
        DG::TEXTURE_FORMAT GetIntermediateDepthbufferFormat() const;
        int GetMaxAnisotropy() const;
        int GetMSAASamples() const;
    };

    class TextureLoaderModule : public IRenderModule {
    private:
        RealtimeGraphics& mGraphics;

    public:
        std::unique_ptr<Task> Startup(Renderer* renderer) override;
        std::unique_ptr<Task> LoadResources(Frame* frame) override;
        void NewFrame(Frame* frame) override;
        TaskNode GenerateTaskNode(
            Future<RenderParams> params,
            Future<RenderView> viewParams) override;
        bool AllowMultithreading() const override;
        bool IsLoadingModule() const override;
        entt::meta_type GetType() const override;
    };

    class GeometryLoaderModule : public IRenderModule {
    private:
        RealtimeGraphics& mGraphics;

    public:
        std::unique_ptr<Task> Startup(Renderer* renderer) override;
        std::unique_ptr<Task> LoadResources(Frame* frame) override;
        void NewFrame(Frame* frame) override;
        TaskNode GenerateTaskNode(
            Future<RenderParams> params,
            Future<RenderView> viewParams) override;
        bool AllowMultithreading() const override;
        bool IsLoadingModule() const override;
        entt::meta_type GetType() const override;
    };

    class SkyboxModule : public IRenderModule {
    private:
        Handle<DG::IShader> mVertexShader;
        Handle<DG::IShader> mPixelShader;
        Handle<DG::IPipelineState> mPipeline;
        Handle<DG::IShaderResourceBinding> mResourceBinding;

    public:
        inline SkyboxModule(
            TextureLoaderModule* textureModule) {
        }

        std::unique_ptr<Task> Startup(Renderer* renderer) override;
        std::unique_ptr<Task> LoadResources(Frame* frame) override;
        void NewFrame(Frame* frame) override;
        TaskNode GenerateTaskNode(
            Future<RenderParams> params,
            Future<RenderView> viewParams) override;
        bool AllowMultithreading() const override;
        bool IsLoadingModule() const override;
        entt::meta_type GetType() const override;
    };

    struct TexturedStaticMeshInstance {
        DG::float4x4 mWorld;
        DG::float4x4 mView;
        DG::float4x4 mProjection;
    };

    class TexturedStaticMeshModule : 
        public IRenderModule,
        public IStaticMeshVertexFormatProvider {
    private:
        Handle<DG::IShader> mVertexShader;
        Handle<DG::IShader> mPixelShader;
        Handle<DG::IPipelineState> mPipeline;
        DynamicUniformBuffer<HLSL::StaticMeshInstanceData> mInstanceData;
        std::unordered_map<Material*, Handle<DG::IShaderResourceBinding>> mBindings;

        Handle<DG::IShaderResourceBinding> GenerateBinding(Material* material) {
        }

    public:
        inline TexturedStaticMeshModule(
            TextureLoaderModule* textureModule,
            GeometryLoaderModule* geometryModule) {
        }

        std::unique_ptr<Task> Startup(Renderer* renderer) override;
        std::unique_ptr<Task> LoadResources(Frame* frame) override;
        void NewFrame(Frame* frame) override;
        TaskNode GenerateTaskNode(
            Future<RenderParams> params,
            Future<RenderView> viewParams) override;
        bool AllowMultithreading() const override;
        bool IsLoadingModule() const override;
        entt::meta_type GetType() const override;
        VertexLayout GetStaticMeshLayout() const override;
    };
}