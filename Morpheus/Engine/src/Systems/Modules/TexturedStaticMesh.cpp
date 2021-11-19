#include <Engine/Systems/Renderer.hpp>

#include <Engine/Resources/Shader.hpp>

namespace Morpheus {
    std::unique_ptr<Task> TexturedStaticMeshModule::Startup(Renderer* renderer) {

        if (!renderer->SetStaticMeshFormatProvider(this)) {
            throw std::runtime_error("Another static mesh format provider already exists!");
        }

        ShaderPreprocessorConfig config;
        config.mDefines["IS_INSTANCED"] = "0";
        config.mDefines["USE_SH"] = "1";

        LoadParams<RawShader> vsParams(
            "StaticMesh/Vertex.vsh",
            DG::SHADER_TYPE_VERTEX,
            "Textured Static Mesh VS", 
            config);

        LoadParams<RawShader> psParams(
            "StaticMesh/Textured.psh",
            DG::SHADER_TYPE_PIXEL,
            "Textured Static Mesh PS",
            config);

        auto vs = LoadShaderHandle(renderer->Graphics().Device(), vsParams);
        auto ps = LoadShaderHandle(renderer->Graphics().Device(), psParams);

        auto spawnPipeline = FunctionPrototype<
            Future<Handle<DG::IShader>>, 
            Future<Handle<DG::IShader>>,
            Promise<Handle<DG::IPipelineState>>>(
            [renderer, this](const TaskParams& params, 
                Future<Handle<DG::IShader>> vs, 
                Future<Handle<DG::IShader>> ps,
                Promise<Handle<DG::IPipelineState>> output) {     

                auto anisotropyFactor = renderer->GetMaxAnisotropy();
                auto filterType = anisotropyFactor > 1 ? 
                            DG::FILTER_TYPE_ANISOTROPIC : DG::FILTER_TYPE_LINEAR;

                DG::SamplerDesc SamLinearWrapDesc 
                {
                    filterType, filterType, filterType, 
                    DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP
                };

                SamLinearWrapDesc.MaxAnisotropy = anisotropyFactor;

                DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
                DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
                DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

                PSODesc.Name         = "Textured Static Mesh Pipeline";
                PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

                GraphicsPipeline.NumRenderTargets             = 1;
                GraphicsPipeline.RTVFormats[0]                = renderer->GetIntermediateFramebufferFormat();
                GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
                GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
                GraphicsPipeline.DepthStencilDesc.DepthFunc   = DG::COMPARISON_FUNC_LESS;
                GraphicsPipeline.DSVFormat 					  = renderer->GetIntermediateDepthbufferFormat();

                // Number of MSAA samples
                GraphicsPipeline.SmplDesc.Count = (DG::Uint8)renderer->GetMSAASamples();

                auto layout = GetStaticMeshLayout();
                auto& layoutElements = layout.mElements;

                GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
                GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

                PSOCreateInfo.pVS = vs.Get();
                PSOCreateInfo.pPS = ps.Get();

                PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
                
                std::vector<DG::ShaderResourceVariableDesc> vars;

                vars.emplace_back(DG::ShaderResourceVariableDesc(
                    DG::SHADER_TYPE_VERTEX, "Globals",
                    DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC));

                vars.emplace_back(DG::ShaderResourceVariableDesc(
                    DG::SHADER_TYPE_VERTEX, "Instance",
                    DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC));

                PSODesc.ResourceLayout.NumVariables = vars.size();
		        PSODesc.ResourceLayout.Variables    = &vars[0];

                std::vector<DG::ImmutableSamplerDesc> ImtblSamplers;

                ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
                    DG::SHADER_TYPE_PIXEL, "mAlbedo_sampler", SamLinearWrapDesc
                });

                PSODesc.ResourceLayout.NumImmutableSamplers = ImtblSamplers.size();
		        PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSamplers[0];

                // Create the pipeline
                Handle<DG::IPipelineState> pipeline;
                renderer->Graphics().Device()->CreateGraphicsPipelineState(PSOCreateInfo, pipeline.Ref());
                output.Set(pipeline);
            }
        );
    }

    std::unique_ptr<Task> TexturedStaticMeshModule::LoadResources(Frame* frame) {
        return nullptr;
    }

    void TexturedStaticMeshModule::NewFrame(Frame* frame) {
        
    }

    TaskNode TexturedStaticMeshModule::GenerateTaskNode(Future<RenderParams> future) {

    }

    bool TexturedStaticMeshModule::AllowMultithreading() const {
        return false;
    }

    bool TexturedStaticMeshModule::IsLoadingModule() const {
        return false;
    }

    entt::meta_type TexturedStaticMeshModule::GetType() const {
        return entt::resolve<TexturedStaticMeshModule>();
    }
}