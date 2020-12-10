#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/PipelineResource.hpp>

namespace Morpheus {
	template <
		bool bDefaultUseIBL,
		bool bDefaultUseAO,
		bool bDefaultUseEmissive
	>
	void CreatePBRStaticMeshPipeline(
		DG::IRenderDevice* device,
		ResourceManager* manager, 
		Renderer* renderer, 
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {
			
		ShaderPreprocessorConfig newOverrides;
		if (overrides) {
			newOverrides.mDefines = overrides->mDefines;
		}
		
		bool bUseIBL = bDefaultUseIBL;
		auto itUseIBL = newOverrides.mDefines.find("GLTF_PBR_USE_IBL");
		if (itUseIBL != newOverrides.mDefines.end()) {
			if (itUseIBL->second == "0") {
				bUseIBL = false;
			} else if (itUseIBL->second == "1") {
				bUseIBL = true;
			} else {
				throw std::runtime_error("GLTF_PBR_USE_IBL macro has invalid value!");
			}
		} else {
			newOverrides.mDefines["GLTF_PBR_USE_IBL"] = bUseIBL ? "1" : "0";
		}

		bool bUseAO = bDefaultUseAO;
		auto itUseAO = newOverrides.mDefines.find("GLTF_PBR_USE_AO");
		if (itUseAO != newOverrides.mDefines.end()) {
			if (itUseAO->second == "0") {
				bUseAO = false;
			} else if (itUseAO->second == "1") {
				bUseAO = true;
			} else {
				throw std::runtime_error("GLTF_PBR_USE_AO macro has invalid value!");
			}
		} else {
			newOverrides.mDefines["GLTF_PBR_USE_AO"] = bUseAO ? "1" : "0";
		}

		bool bUseEmissive = bDefaultUseEmissive;
		auto itUseEmissive = newOverrides.mDefines.find("GLTF_PBR_USE_EMISSIVE");
		if (itUseEmissive != newOverrides.mDefines.end()) {
			if (itUseEmissive->second == "0") {
				bUseEmissive = false;
			} else if (itUseEmissive->second == "1") {
				bUseEmissive = true;
			} else {
				throw std::runtime_error("GLTF_PBR_USE_EMISSIVE macro has invalid value!");
			}
		} else {
			newOverrides.mDefines["GLTF_PBR_USE_EMISSIVE"] = bUseEmissive ? "1" : "0";
		}

		auto pbrStaticMeshVS = LoadShader(device, 
			DG::SHADER_TYPE_VERTEX,
			"internal/StaticMeshPBR.vsh",
			"StaticMesh PBR VS",
			"main",
			overrides,
			shaderLoader);

		auto pbrStaticMeshPS = LoadShader(device,
			DG::SHADER_TYPE_PIXEL,
			"internal/StaticMeshPBR.psh",
			"StaticMesh PBR PS",
			"main",
			overrides,
			shaderLoader);

		DG::SamplerDesc SamLinearClampDesc
		{
			DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

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

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 2, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

			DG::LayoutElement(3, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(4, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(6, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
		};

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = pbrStaticMeshVS;
		PSOCreateInfo.pPS = pbrStaticMeshPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		
		std::vector<DG::ShaderResourceVariableDesc> Vars;

		if (bUseIBL) {
			Vars.emplace_back(DG::ShaderResourceVariableDesc{
				DG::SHADER_TYPE_PIXEL, 
				"g_IrradianceMap", 
				DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
			Vars.emplace_back(DG::ShaderResourceVariableDesc{
				DG::SHADER_TYPE_PIXEL, 
				"g_PrefilteredEnvMap", 
				DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
		}
		
		// clang-format off
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"g_BRDF_LUT", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"g_ColorMap", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"g_RoughnessMap", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"g_MetallicMap", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"g_NormalMap", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});

		if (bUseAO) {
			Vars.emplace_back(DG::ShaderResourceVariableDesc{
				DG::SHADER_TYPE_PIXEL, 
				"g_AOMap", 
				DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		}

		if (bUseEmissive) {
			Vars.emplace_back(DG::ShaderResourceVariableDesc{
				DG::SHADER_TYPE_PIXEL, 
				"g_EmissiveMap", 
				DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		}

		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"cbGLTFAttribs", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});

		// clang-format on
		PSODesc.ResourceLayout.NumVariables = Vars.size();
		PSODesc.ResourceLayout.Variables    = &Vars[0];

		std::vector<DG::ImmutableSamplerDesc> ImtblSamplers;
		if (bUseIBL) {
			ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
				DG::SHADER_TYPE_PIXEL, "g_IrradianceMap_sampler", SamLinearClampDesc
			});
			ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
				DG::SHADER_TYPE_PIXEL, "g_PrefilteredEnvMap_sampler", SamLinearClampDesc
			});
			ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
				DG::SHADER_TYPE_PIXEL, "g_BRDF_LUT_sampler", SamLinearClampDesc
			});
		}

		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "g_ColorMap_sampler", SamLinearClampDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "g_RoughnessMap_sampler", SamLinearClampDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "g_MetallicMap_sampler", SamLinearClampDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "g_NormalMap_sampler", SamLinearClampDesc
		});

		if (bUseAO) {
			ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
				DG::SHADER_TYPE_PIXEL, "g_AOMap_sampler", SamLinearClampDesc
			});
		}
		
		if (bUseEmissive) {
			ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
				DG::SHADER_TYPE_PIXEL, "g_EmissiveMap_sampler", SamLinearClampDesc
			});
		}

		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = ImtblSamplers.size();
		PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSamplers[0];
		
		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);
		result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals")->Set(renderer->GetGlobalsBuffer());
		result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "Globals")->Set(renderer->GetGlobalsBuffer());

		if (bUseIBL) {
			result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "g_BRDF_LUT")->Set(
				renderer->GetLUTShaderResourceView()
			);
		}

		pbrStaticMeshVS->Release();
		pbrStaticMeshPS->Release();

		VertexAttributeIndices indices;
		indices.mPosition = 0;
		indices.mNormal = 1;
		indices.mUV = 2;

		into->SetAll(
			result,
			layoutElements,
			indices,
			InstancingType::INSTANCED_STATIC_TRANSFORMS);
	}
}