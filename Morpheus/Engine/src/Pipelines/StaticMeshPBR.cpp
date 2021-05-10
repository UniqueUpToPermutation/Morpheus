#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Pipelines/ImageBasedLightingView.hpp>
#include <Engine/Materials/StaticMeshPBRMaterial.hpp>
#include <Engine/Renderer.hpp>
namespace Morpheus {
	Task CreateStaticMeshPBRPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {

		ShaderPreprocessorConfig overridesCopy;
		if (overrides)
			overridesCopy = *overrides;

		struct Data {
			bool bUseSH;
			bool bUseIBL;
			ShaderResource* mPbrStaticMeshVSResource;
			ShaderResource* mPbrStaticMeshPSResource;

			~Data() {
				if (mPbrStaticMeshVSResource)
					mPbrStaticMeshVSResource->Release();
				if (mPbrStaticMeshPSResource)
					mPbrStaticMeshPSResource->Release();
			}
		};

		Task task([device, manager, renderer, into, 
			overrides = std::move(overridesCopy), data = Data()](const TaskParams& e) mutable {

			if (e.mTask->SubTask()) {
				data.bUseSH = renderer->GetUseSHIrradiance();
				data.bUseIBL = renderer->GetUseIBL();

				auto itUseIBL = overrides.mDefines.find("USE_IBL");
				if (itUseIBL != overrides.mDefines.end()) {
					if (itUseIBL->second == "0" || itUseIBL->second == "false") {
						data.bUseIBL = false;
					} else if (itUseIBL->second == "1" || itUseIBL->second == "true") {
						data.bUseIBL = true;
					} else {
						throw std::runtime_error("USE_IBL macro has invalid value!");
					}
				} else {
					overrides.mDefines["USE_IBL"] = data.bUseIBL ? "1" : "0";
				}

				auto itUseSH = overrides.mDefines.find("USE_SH");
				if (itUseSH != overrides.mDefines.end()) {
					if (itUseSH->second == "0" || itUseSH->second == "false") {
						data.bUseSH = false;
					} else if (itUseSH->second == "1" || itUseSH->second == "true") {
						data.bUseSH = true;
					} else {
						throw std::runtime_error("USE_IBL macro has invalid value!");
					}
				} else {
					overrides.mDefines["USE_SH"] = data.bUseSH ? "1" : "0";
				}

				LoadParams<ShaderResource> vsParams(
					"internal/StaticMesh.vsh",
					DG::SHADER_TYPE_VERTEX,
					"StaticMesh VS",
					&overrides,
					"main"
				);

				LoadParams<ShaderResource> psParams(
					"internal/PBR.psh",
					DG::SHADER_TYPE_PIXEL,
					"PBR PS",
					&overrides,
					"main"
				);

				e.mQueue->AdoptAndTrigger(manager->LoadTask<ShaderResource>(vsParams, &data.mPbrStaticMeshVSResource));
				e.mQueue->AdoptAndTrigger(manager->LoadTask<ShaderResource>(psParams, &data.mPbrStaticMeshPSResource));

				if (e.mTask->In().Lock()
					.Connect(&data.mPbrStaticMeshVSResource->GetLoadBarrier()->mOut)
					.Connect(&data.mPbrStaticMeshPSResource->GetLoadBarrier()->mOut)
					.ShouldWait())
					return TaskResult::WAITING;
			}

			// Spawn pipeline on main thread
			if (e.mTask->SubTask()) {
				auto pbrStaticMeshVS = data.mPbrStaticMeshVSResource->GetShader();
				auto pbrStaticMeshPS = data.mPbrStaticMeshPSResource->GetShader();

				auto anisotropyFactor = renderer->GetMaxAnisotropy();
				auto filterType = anisotropyFactor > 1 ? DG::FILTER_TYPE_ANISOTROPIC : DG::FILTER_TYPE_LINEAR;

				DG::SamplerDesc SamLinearClampDesc
				{
					filterType, filterType, filterType, 
					DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
				};

				DG::SamplerDesc SamLinearWrapDesc 
				{
					filterType, filterType, filterType, 
					DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP
				};

				SamLinearClampDesc.MaxAnisotropy = anisotropyFactor;
				SamLinearWrapDesc.MaxAnisotropy = anisotropyFactor;

				DG::IPipelineState* result = nullptr;

				// Create Irradiance Pipeline
				DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
				DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
				DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

				PSODesc.Name         = "Static Mesh PBR Pipeline";
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

				uint stride = 12 * sizeof(float);

				std::vector<DG::LayoutElement> layoutElements = {
					DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
					DG::LayoutElement(1, 0, 3, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
					DG::LayoutElement(2, 0, 2, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
					DG::LayoutElement(3, 0, 3, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

					DG::LayoutElement(4, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
					DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
					DG::LayoutElement(6, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
					DG::LayoutElement(7, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
				};

				GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
				GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

				PSOCreateInfo.pVS = pbrStaticMeshVS;
				PSOCreateInfo.pPS = pbrStaticMeshPS;

				PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
				
				std::vector<DG::ShaderResourceVariableDesc> Vars;
				
				// clang-format off
				Vars.emplace_back(DG::ShaderResourceVariableDesc{
					DG::SHADER_TYPE_PIXEL, 
					"mAlbedo", 
					DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
				Vars.emplace_back(DG::ShaderResourceVariableDesc{
					DG::SHADER_TYPE_PIXEL, 
					"mMetallic", 
					DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
				Vars.emplace_back(DG::ShaderResourceVariableDesc{
					DG::SHADER_TYPE_PIXEL, 
					"mRoughness", 
					DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
				Vars.emplace_back(DG::ShaderResourceVariableDesc{
					DG::SHADER_TYPE_PIXEL, 
					"mNormalMap", 
					DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});

				if (data.bUseIBL) {
					if (data.bUseSH) {
						Vars.emplace_back(DG::ShaderResourceVariableDesc{
							DG::SHADER_TYPE_PIXEL, 
							"IrradianceSH", 
							DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
					}
					else {
						Vars.emplace_back(DG::ShaderResourceVariableDesc{
							DG::SHADER_TYPE_PIXEL, 
							"mIrradianceMap", 
							DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
					}
					Vars.emplace_back(DG::ShaderResourceVariableDesc{
						DG::SHADER_TYPE_PIXEL, 
						"mPrefilteredEnvMap", 
						DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
						// clang-format off
					Vars.emplace_back(DG::ShaderResourceVariableDesc{
						DG::SHADER_TYPE_PIXEL, 
						"mBRDF_LUT", 
						DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC});
				}

				// clang-format on
				PSODesc.ResourceLayout.NumVariables = Vars.size();
				PSODesc.ResourceLayout.Variables    = &Vars[0];

				std::vector<DG::ImmutableSamplerDesc> ImtblSamplers;

				ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
					DG::SHADER_TYPE_PIXEL, "mAlbedo_sampler", SamLinearWrapDesc
				});
				ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
					DG::SHADER_TYPE_PIXEL, "mRoughness_sampler", SamLinearWrapDesc
				});
				ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
					DG::SHADER_TYPE_PIXEL, "mMetallic_sampler", SamLinearWrapDesc
				});
				ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
					DG::SHADER_TYPE_PIXEL, "mNormalMap_sampler", SamLinearWrapDesc
				});

				if (data.bUseIBL) {
					if (!data.bUseSH) {
						ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
							DG::SHADER_TYPE_PIXEL, "mIrradianceMap_sampler", SamLinearClampDesc
						});
					}
					ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
						DG::SHADER_TYPE_PIXEL, "mPrefilteredEnvMap_sampler", SamLinearClampDesc
					});
					ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
						DG::SHADER_TYPE_PIXEL, "mBRDF_LUT_sampler", SamLinearClampDesc
					});
				}

				// clang-format on
				PSODesc.ResourceLayout.NumImmutableSamplers = ImtblSamplers.size();
				PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSamplers[0];
				
				device->CreateGraphicsPipelineState(PSOCreateInfo, &result);

				auto globalsVar =result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals");
				if (globalsVar)
					globalsVar->Set(renderer->GetGlobalsBuffer());
				globalsVar = result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "Globals");
				if (globalsVar)
					globalsVar->Set(renderer->GetGlobalsBuffer());

				if (data.bUseIBL) {
					auto lutVar = result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "mBRDF_LUT");
					if (lutVar)
						lutVar->Set(renderer->GetLUTShaderResourceView());
				}

				VertexLayout layout;
				layout.mElements = std::move(layoutElements);
				layout.mPosition = 0;
				layout.mNormal = 1;
				layout.mUV = 2;
				layout.mTangent = 3;
				layout.mStride = 12 * sizeof(float);

				into->SetAll(
					result,
					GenerateSRBs(result, renderer),
					layout,
					InstancingType::INSTANCED_STATIC_TRANSFORMS);

				into->AddView<ImageBasedLightingView>(into);
				into->AddView<StaticMeshPBRPipelineView>(into);
			}

			return TaskResult::FINISHED;
		},
		"Upload Static Mesh PBR Pipeline",
		TaskType::UNSPECIFIED,
		ASSIGN_THREAD_MAIN);
			
		return task;
	}
}