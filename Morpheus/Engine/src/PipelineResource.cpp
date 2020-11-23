#include <Engine/PipelineResource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/ShaderLoader.hpp>

#include <fstream>

namespace Morpheus {
	PipelineResource* PipelineResource::ToPipeline() {
		return this;
	}

	void PipelineResource::Init(DG::IPipelineState* state,
		std::vector<DG::LayoutElement> layoutElements,
		VertexAttributeIndices attributeIndices) {
		mState = state;
		mVertexLayout = layoutElements;
		mAttributeIndices = attributeIndices;
	}

	DG::TEXTURE_FORMAT PipelineLoader::ReadTextureFormat(const std::string& str) {
		if (str == "SWAP_CHAIN_COLOR_BUFFER_FORMAT") {
			return mManager->GetParent()->GetSwapChain()
				->GetDesc().ColorBufferFormat;
		} else if (str == "SWAP_CHAIN_DEPTH_BUFFER_FORMAT") {
			return mManager->GetParent()->GetSwapChain()
				->GetDesc().DepthBufferFormat;
		}
		else if (str == "TEX_FORMAT_RGBA8_UNORM") {
			return DG::TEX_FORMAT_RGBA8_UNORM;
		}
		else if (str == "TEX_FORMAT_RGBA16_FLOAT") {
			return DG::TEX_FORMAT_RGBA16_FLOAT;
		}
		else if (str == "TEX_FORMAT_RGBA32_FLOAT") {
			return DG::TEX_FORMAT_RGBA32_FLOAT;
		}
		else if (str == "TEX_FORMAT_R8_UNORM") {
			return DG::TEX_FORMAT_R8_UNORM;
		}
		else if (str == "TEX_FORMAT_R16_FLOAT") {
			return DG::TEX_FORMAT_R16_FLOAT;
		}
		else if (str == "TEX_FORMAT_R32_FLOAT") {
			return DG::TEX_FORMAT_R32_FLOAT;
		}
		else if (str == "TEX_FORMAT_RG8_UNORM") {
			return DG::TEX_FORMAT_RG8_UNORM;
		}
		else if (str == "TEX_FORMAT_RG16_FLOAT") {
			return DG::TEX_FORMAT_RG16_FLOAT;
		}
		else if (str == "TEX_FORMAT_RG32_FLOAT") {
			return DG::TEX_FORMAT_RG32_FLOAT;
		}
		else {
			throw std::runtime_error("Unrecognized texture format!");
		}
	}

	DG::PRIMITIVE_TOPOLOGY PipelineLoader::ReadPrimitiveTopology(const std::string& str) {
		if (str == "PRIMITIVE_TOPOLOGY_TRIANGLE_LIST") {
			return DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
		else if (str == "PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP") {
			return DG::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		}
		else if (str == "PRIMTIIVE_TOPOLOGY_LINE_LIST") {
			return DG::PRIMITIVE_TOPOLOGY_LINE_LIST;
		}
		else if (str == "PRIMITIVE_TOPOLOGY_LINE_STRIP") {
			return DG::PRIMITIVE_TOPOLOGY_LINE_STRIP;
		}
		else if (str == "PRIMITIVE_TOPOLOGY_POINT_LIST") {
			return DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
		}
		else {
			throw std::runtime_error("Unrecognized primitive topology!");
		}
	}

	DG::CULL_MODE PipelineLoader::ReadCullMode(const std::string& str) {
		if (str == "CULL_MODE_NONE") {
			return DG::CULL_MODE_NONE;
		} else if (str == "CULL_MODE_BACK") {
			return DG::CULL_MODE_BACK;
		} else if (str == "CULL_MODE_FRONT") {
			return DG::CULL_MODE_FRONT;
		} else {
			throw std::runtime_error("Cull mode not recognized!");
		}
	}

	DG::FILL_MODE PipelineLoader::ReadFillMode(const std::string& str) {
		if (str == "FILL_MODE_SOLID") {
			return DG::FILL_MODE_SOLID;
		} else if (str == "FILL_MODE_WIREFRAME") {
			return DG::FILL_MODE_WIREFRAME;
		} else {
			throw std::runtime_error("Fill mode not recognized!");
		}
	}

	DG::STENCIL_OP PipelineLoader::ReadStencilOp(const std::string& str) {
		if (str == "STENCIL_OP_DECR_SAT") {
			return DG::STENCIL_OP_DECR_SAT;
		} else if (str == "STENCIL_OP_INCR_SAT") {
			return DG::STENCIL_OP_INCR_SAT;
		} else if (str == "STENCIL_OP_DECR_WRAP") {
			return DG::STENCIL_OP_DECR_WRAP;
		} else if (str == "STENCIL_OP_INVERT") {
			return DG::STENCIL_OP_INVERT;
		} else if (str == "STENCIL_OP_KEEP") {
			return DG::STENCIL_OP_KEEP;
		} else if (str == "STENCIL_OP_REPLACE") {
			return DG::STENCIL_OP_REPLACE;
		} else if (str == "STENCIL_OP_ZERO") {
			return DG::STENCIL_OP_ZERO;
		} else {
			throw std::runtime_error("Stencil op not recognized!");
		}
	}

	DG::COMPARISON_FUNCTION PipelineLoader::ReadComparisonFunc(const std::string& str) {
		if (str == "COMPARISON_FUNC_ALWAYS") {
			return DG::COMPARISON_FUNC_ALWAYS;
		} else if (str == "COMPARISON_FUNC_EQUAL") {
			return DG::COMPARISON_FUNC_EQUAL;
		} else if (str == "COMPARISON_FUNC_GREATER") {
			return DG::COMPARISON_FUNC_GREATER;
		} else if (str == "COMPARISON_FUNC_GREATER_EQUAL") {
			return DG::COMPARISON_FUNC_GREATER_EQUAL;
		} else if (str == "COMPARISON_FUNC_LESS") {
			return DG::COMPARISON_FUNC_LESS;
		} else if (str == "COMPARISON_FUNC_LESS_EQUAL") {
			return DG::COMPARISON_FUNC_LESS_EQUAL;
		} else if (str == "COMPARISON_FUNC_NEVER") {
			return DG::COMPARISON_FUNC_NEVER;
		} else if (str == "COMPARISON_FUNC_NOT_EQUAL") {
			return DG::COMPARISON_FUNC_NOT_EQUAL;
		} else {
			throw std::runtime_error("Comparison function not recognized!");
		}
	}

	void PipelineLoader::ReadStencilOpDesc(const nlohmann::json& json, DG::StencilOpDesc* desc) {
		if (json.contains("StencilFunc")) {
			desc->StencilFunc = ReadComparisonFunc(json.value("StencilFunc", ""));
		}
		if (json.contains("StencilDepthFailOp")) {
			desc->StencilDepthFailOp = ReadStencilOp(json.value("StencilDepthFailOp", ""));
		}
		if (json.contains("StencilFailOp")) {
			desc->StencilFailOp = ReadStencilOp(json.value("StenilFailOp", ""));
		}
		if (json.contains("StencilPassOp")) {
			desc->StencilPassOp = ReadStencilOp(json.value("StencilPassOp", ""));
		}
	}

	DG::SHADER_TYPE ReadShaderType(const std::string& str) {
		if (str == "SHADER_TYPE_AMPLIFICATION") {
			return DG::SHADER_TYPE_AMPLIFICATION;
		} else if (str == "SHADER_TYPE_COMPUTE") {
			return DG::SHADER_TYPE_COMPUTE;
		} else if (str == "SHADER_TYPE_DOMAIN") {
			return DG::SHADER_TYPE_DOMAIN;
		} else if (str == "SHADER_TYPE_GEOMETRY") {
			return DG::SHADER_TYPE_GEOMETRY;
		} else if (str == "SHADER_TYPE_HULL") {
			return DG::SHADER_TYPE_HULL;
		} else if (str == "SHADER_TYPE_LAST") {
			return DG::SHADER_TYPE_LAST;
		} else if (str == "SHADER_TYPE_MESH") {
			return DG::SHADER_TYPE_MESH;
		} else if (str == "SHADER_TYPE_PIXEL") {
			return DG::SHADER_TYPE_PIXEL;
		} else if (str == "SHADER_TYPE_VERTEX") {
			return DG::SHADER_TYPE_VERTEX;
		} else {
			throw std::runtime_error("Shader type not recognized!");
		}
	}

	std::vector<DG::LayoutElement> PipelineLoader::ReadLayoutElements(const nlohmann::json& json) {
		if (!json.is_array()) {
			throw std::runtime_error("Layout element json is not array!");
		}

		std::vector<DG::LayoutElement> result;

		for (const auto& item : json) {
			result.emplace_back(ReadLayoutElement(item));
		}

		return result;
	}

	DG::LayoutElement PipelineLoader::ReadLayoutElement(const nlohmann::json& json) {
		DG::LayoutElement elem;
		elem.InputIndex = json.value("InputIndex", elem.InputIndex);
		elem.BufferSlot = json.value("BufferSlot", elem.BufferSlot);
		elem.NumComponents = json.value("NumComponents", elem.NumComponents);
		elem.ValueType = ReadValueType(json["ValueType"]);
		elem.IsNormalized = json.value("IsNormalized", false);

		if (json.contains("Frequency")) {
			std::string str;
			json["Frequency"].get_to(str);
			elem.Frequency = ReadInputElementFrequency(str);
		}

		return elem;
	}

	DG::VALUE_TYPE PipelineLoader::ReadValueType(const nlohmann::json& json) {
		std::string valueType;
		json.get_to(valueType);

		if (valueType == "VT_FLOAT16") {
			return DG::VT_FLOAT16;
		} else if (valueType == "VT_FLOAT32") {
			return DG::VT_FLOAT32;
		} else if (valueType == "VT_INT32") {
			return DG::VT_INT32;
		} else if (valueType == "VT_INT16") {
			return DG::VT_INT16;
		} else if (valueType == "VT_INT8") {
			return DG::VT_INT8;
		} else if (valueType == "VT_UINT32") {
			return DG::VT_UINT32;
		} else if (valueType == "VT_UINT16") {
			return DG::VT_UINT16;
		} else if (valueType == "VT_UINT8") {
			return DG::VT_UINT8;
		} else {
			throw std::runtime_error("ValueType not recognized!");
		}
	}

	VertexAttributeIndices PipelineLoader::ReadVertexAttributes(const nlohmann::json& json) {
		VertexAttributeIndices attribs;
		attribs.mPosition 	= json.value("Position", attribs.mPosition);
		attribs.mUV 		= json.value("UV", attribs.mUV);
		attribs.mNormal 	= json.value("Normal", attribs.mNormal);
		attribs.mTangent 	= json.value("Tangent", attribs.mTangent);
		attribs.mBitangent 	= json.value("Bitangent", attribs.mBitangent);
		return attribs;
	}

	DG::SHADER_RESOURCE_VARIABLE_TYPE PipelineLoader::ReadShaderResourceVariableType(const nlohmann::json& json) {
		std::string defaultVarType;
		json.get_to(defaultVarType);
		if (defaultVarType == "SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC") {
			return DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
		} else if (defaultVarType == "SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE") {
			return DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
		} else if (defaultVarType == "SHADER_RESOURCE_VARIABLE_TYPE_STATIC") {
			return DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		} else {
			throw std::runtime_error("Unrecognized shader resource variable type!");
		}
	}

	DG::SHADER_TYPE PipelineLoader::ReadShaderStages(const nlohmann::json& json) {
		DG::SHADER_TYPE t = (DG::SHADER_TYPE)0u;
		for (const auto& item : json) {
			DG::SHADER_TYPE t_ = ReadShaderType(item);
			t |= t_;
		}
		return t;
	}

	DG::TEXTURE_ADDRESS_MODE PipelineLoader::ReadTextureAddressMode(const nlohmann::json& json) {
		std::string s;
		json.get_to(s);

		if (s == "TEXTURE_ADDRESS_WRAP")
			return DG::TEXTURE_ADDRESS_WRAP;
		else if (s == "TEXTURE_ADDRESS_MIRROR")
			return DG::TEXTURE_ADDRESS_MIRROR;
		else if (s == "TEXTURE_ADDRESS_MIRROR_ONCE")
			return DG::TEXTURE_ADDRESS_MIRROR_ONCE;
		else if (s == "TEXTURE_ADDRESS_CLAMP")
			return DG::TEXTURE_ADDRESS_CLAMP;
		else if (s == "TEXTURE_ADDRESS_BORDER")
			return DG::TEXTURE_ADDRESS_BORDER;
		else 
			throw std::runtime_error("Unrecognized texture address mode!");
	}

	DG::FILTER_TYPE PipelineLoader::ReadFilterType(const nlohmann::json& json) {
		std::string s;
		json.get_to(s);

		if (s == "RendererDefault") {
			return mManager->GetParent()->GetRenderer()->GetDefaultFilter();
		} else if (s == "FILTER_TYPE_LINEAR") {
			return DG::FILTER_TYPE_LINEAR;
		} else if (s == "FILTER_TYPE_ANISOTROPIC") {
			return DG::FILTER_TYPE_ANISOTROPIC;
		} else if (s == "FILTER_TYPE_POINT") {
			return DG::FILTER_TYPE_POINT;
		} else {
			throw std::runtime_error("Unrecognized filter type!");
		}
	}

	DG::INPUT_ELEMENT_FREQUENCY PipelineLoader::ReadInputElementFrequency(const std::string& str) {
		if (str == "INPUT_ELEMENT_FREQUENCY_PER_VERTEX") {
			return DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
		} else if (str == "INPUT_ELEMENT_FREQUENCY_PER_INSTANCE") {
			return DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;
		} else {
			throw std::runtime_error("Unrecognized filter type!");
		}
	}

	DG::SamplerDesc PipelineLoader::ReadSamplerDesc(const nlohmann::json& json) {
		DG::SamplerDesc desc;

		if (json.contains("AddressU")) {
			desc.AddressU = ReadTextureAddressMode(json["AddressU"]);
		}
		if (json.contains("AddressV")) {
			desc.AddressV = ReadTextureAddressMode(json["AddressV"]);
		}
		if (json.contains("AddressW")) {
			desc.AddressW = ReadTextureAddressMode(json["AddressW"]);
		}

		if (json.contains("MinFilter")) {
			desc.MinFilter = ReadFilterType(json["MinFilter"]);
		}
		if (json.contains("MagFilter")) {
			desc.MagFilter = ReadFilterType(json["MagFilter"]);
		}
		if (json.contains("MipFilter")) {
			desc.MipFilter = ReadFilterType(json["MipFilter"]);
		}

		return desc;
	}

	DG::PipelineResourceLayoutDesc PipelineLoader::ReadResourceLayout(const nlohmann::json& json,
		std::vector<DG::ShaderResourceVariableDesc>* variables,
		std::vector<DG::ImmutableSamplerDesc>* immutableSamplers,
		std::vector<char*>* strings) {
		DG::PipelineResourceLayoutDesc resourceLayout;

		resourceLayout.DefaultVariableType;

		if (json.contains("DefaultVariableType")) {
			resourceLayout.DefaultVariableType = ReadShaderResourceVariableType(json["DefaultVariableType"]);
		}

		if (json.contains("Variables")) {
			const auto& var_json = json["Variables"];

			for (const auto& item : var_json) {
				DG::ShaderResourceVariableDesc varDesc;
				std::string name;

				item["Name"].get_to(name);

				char* str = new char[name.size() + 1];
				memcpy(str, name.c_str(), name.size() + 1);

				strings->emplace_back(str);
				varDesc.Name = str;
				if (item.contains("Type")) {
					varDesc.Type = ReadShaderResourceVariableType(item["Type"]);
				} else {
					varDesc.Type = resourceLayout.DefaultVariableType;
				}
				if (item["ShaderStages"].is_array())
					varDesc.ShaderStages = ReadShaderStages(item["ShaderStages"]);
				else 
					varDesc.ShaderStages = ReadShaderType(item["ShaderStages"]);

				variables->emplace_back(varDesc);
			}
		}

		if (json.contains("ImmutableSamplers")) {
			const auto& var_json = json["ImmutableSamplers"];

			for (const auto& item : var_json) {
				DG::ImmutableSamplerDesc sampDesc;

				std::string name;
				item["Name"].get_to(name);
				
				char* str = new char[name.size() + 1];
				memcpy(str, name.c_str(), name.size() + 1);

				strings->emplace_back(str);

				sampDesc.SamplerOrTextureName = str;
				sampDesc.Desc = ReadSamplerDesc(item);
				sampDesc.Desc.MaxAnisotropy = mManager->GetParent()->GetRenderer()->GetMaxAnisotropy();
				
				if (item["ShaderStages"].is_array())
					sampDesc.ShaderStages = ReadShaderStages(item["ShaderStages"]);
				else 
					sampDesc.ShaderStages = ReadShaderType(item["ShaderStages"]);

				immutableSamplers->emplace_back(sampDesc);
			}
		}

		resourceLayout.ImmutableSamplers = immutableSamplers->data();
		resourceLayout.NumImmutableSamplers = immutableSamplers->size();

		resourceLayout.Variables = variables->data();
		resourceLayout.NumVariables = variables->size();

		return resourceLayout;
	}

	void PipelineLoader::ReadRasterizerDesc(const nlohmann::json& json, 
		DG::RasterizerStateDesc* desc) {
		desc->AntialiasedLineEnable = json.value("AntialiasedLineEnable", desc->AntialiasedLineEnable);
		desc->CullMode = ReadCullMode(json.value("CullMode", "CULL_MODE_BACK"));
		desc->DepthBias = json.value("DepthBias", desc->DepthBias);
		desc->DepthBiasClamp = json.value("DepthBiasClamp", desc->DepthBiasClamp);
		desc->DepthClipEnable = json.value("DepthClipEnable", desc->DepthClipEnable);
		desc->FillMode = ReadFillMode(json.value("FillMode", "FILL_MODE_SOLID"));
		desc->FrontCounterClockwise = json.value("FrontCounterClockwise", desc->FrontCounterClockwise);
		desc->ScissorEnable = json.value("ScissorEnable", desc->ScissorEnable);
		desc->SlopeScaledDepthBias = json.value("SlopeScaledDepthBias", desc->SlopeScaledDepthBias);
	}

	void PipelineLoader::ReadDepthStencilDesc(const nlohmann::json& json, 
		DG::DepthStencilStateDesc* desc) {
		desc->DepthEnable = json.value("DepthEnable", desc->DepthEnable);
		desc->StencilEnable = json.value("StencilEnable", desc->StencilEnable);
		desc->StencilReadMask = json.value("StencilReadMask", desc->StencilReadMask);
		desc->StencilWriteMask = json.value("StencilWriteMask", desc->StencilWriteMask);
		if (json.contains("BackFace")) {
			ReadStencilOpDesc(json["BackFace"], &desc->BackFace);
		}
		if (json.contains("FrontFace")) {
			ReadStencilOpDesc(json["FrontFace"], &desc->FrontFace);
		}
		if (json.contains("DepthFunc")) {
			desc->DepthFunc = ReadComparisonFunc(json.value("DepthFunc", ""));
		}

		desc->DepthWriteEnable = json.value("DepthWriteEnable", desc->DepthWriteEnable);
	}

	void PipelineLoader::Load(const std::string& source, PipelineResource* resource,
		const ShaderPreprocessorConfig* overrides) {

		std::cout << "Loading " << source << "..." << std::endl;

		nlohmann::json json;
		if (!mShaderLoader.TryLoadJson(source, json)) {
			throw std::runtime_error("Failed to load pipeline json file!");
		}

		std::string path = ".";
		auto path_cutoff = source.rfind('/');
		if (path_cutoff != std::string::npos) {
			path = source.substr(0, path_cutoff);
		}

		Load(json, path, resource, overrides);
	}

	void PipelineLoader::Load(const nlohmann::json& json,
		const std::string& path, PipelineResource* resource,
		const ShaderPreprocessorConfig* overrides) {
		auto type = json.value("PipelineType", "PIPELINE_TYPE_GRAPHICS");

		std::vector<DG::IShader*> shaders;

		if (type == "PIPELINE_TYPE_GRAPHICS") {
			std::vector<DG::LayoutElement> layoutElements;
			std::vector<DG::ShaderResourceVariableDesc> variables;
			std::vector<DG::ImmutableSamplerDesc> immutableSamplers;
			std::vector<char*> strings;

			// Read the pipeline description
			DG::GraphicsPipelineStateCreateInfo info = 
				ReadGraphicsInfo(json, &layoutElements, 
				&variables, &immutableSamplers, &strings);

			auto shaderLoad = [&json, &path, &shaders, &overrides, this](
				DG::IShader** result,
				const std::string& shaderMacro) {
				if (json.contains(shaderMacro)) {
					auto json_macro = json[shaderMacro];
					auto shad = LoadShader(json_macro, path, overrides);
					shaders.emplace_back(shad);
					*result = shad;
				}
			};

			VertexAttributeIndices attribIndices;
			if (json.contains("Attributes")) {
				attribIndices = ReadVertexAttributes(json["Attributes"]);
			}

			// Load shaders
			shaderLoad(&info.pVS, "VS");
			shaderLoad(&info.pPS, "PS");
			shaderLoad(&info.pMS, "MS");
			shaderLoad(&info.pHS, "HS");
			shaderLoad(&info.pGS, "GS");
			shaderLoad(&info.pDS, "DS");
			shaderLoad(&info.pAS, "AS");

			DG::IPipelineState* state = nullptr;
			mManager->GetParent()->GetDevice()
				->CreateGraphicsPipelineState(info, &state);

			// Get globals buffer, if it exists
			std::string globalsBuffer = json.value("GlobalsBuffer", "Globals");
			auto variable = state->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, globalsBuffer.c_str());
			if (variable) {	
				auto globalBuffer = mManager->GetParent()->GetRenderer()->GetGlobalsBuffer();
				variable->Set(globalBuffer);
			}

			resource->Init(state, layoutElements, attribIndices);

			for (auto it : strings) {
				delete[] it;
			}

		} else if (type == "PIPELINE_TYPE_COMPUTE") {
			DG::ComputePipelineStateCreateInfo info = 
				ReadComputeInfo(json);
			throw std::runtime_error("Not implemented!");
		} else {
			throw std::runtime_error("Pipeline type not recognized!");
		}

		for (auto shader : shaders) {
			shader->Release();
		}
	}

	DG::ComputePipelineStateCreateInfo PipelineLoader::ReadComputeInfo(const nlohmann::json& json) {
		throw std::runtime_error("Not implemented yet!");
	}

	DG::GraphicsPipelineStateCreateInfo PipelineLoader::ReadGraphicsInfo(const nlohmann::json& json,
		std::vector<DG::LayoutElement>* layoutElements,
		std::vector<DG::ShaderResourceVariableDesc>* variables,
		std::vector<DG::ImmutableSamplerDesc>* immutableSamplers,
		std::vector<char*>* strings) {

		DG::GraphicsPipelineStateCreateInfo info;

		std::string name = json.value("Name", "Unnammed Pipeline");

		char* str = new char[name.size() + 1];
		memcpy(str, name.c_str(), name.size() + 1);
		strings->emplace_back(str);

		info.PSODesc.Name = str;

		std::string pipelineType = json.value("PipelineType", "PIPELINE_TYPE_GRAPHICS");
		
		if (pipelineType == "PIPELINE_TYPE_GRAPHICS") {
			info.PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;
		} else if (pipelineType == "PIPELINE_TYPE_COMPUTE") {
			info.PSODesc.PipelineType = DG::PIPELINE_TYPE_COMPUTE;
		}

		info.GraphicsPipeline.NumRenderTargets = json.value("NumRenderTargets", 
			info.GraphicsPipeline.NumRenderTargets);
		
		if (json.contains("RTVFormats")) {
			auto rtv_formats = json["RTVFormats"];
			assert(rtv_formats.is_array());
			std::vector<std::string> formats;
			rtv_formats.get_to(formats);

			for (uint i = 0; i < formats.size(); ++i) {
				auto& format = formats[i];
				info.GraphicsPipeline.RTVFormats[i] = ReadTextureFormat(format);
			}
		}

		if (json.contains("DSVFormat")) {
			std::string format;
			json["DSVFormat"].get_to(format);

			info.GraphicsPipeline.DSVFormat = ReadTextureFormat(format);
		}

		std::string primitiveTopology = json.value("PrimitiveTopology", 
			"PRIMITIVE_TOPOLOGY_TRIANGLE_LIST");
		info.GraphicsPipeline.PrimitiveTopology = 
			ReadPrimitiveTopology(primitiveTopology);

		if (json.contains("DepthStencilDesc")) {
			ReadDepthStencilDesc(json["DepthStencilDesc"], 
				&info.GraphicsPipeline.DepthStencilDesc);
		}

		if (json.contains("RasterizerDesc")) {
			ReadRasterizerDesc(json["RasterizerDesc"],
				&info.GraphicsPipeline.RasterizerDesc);
		}

		if (json.contains("InputLayout")) {
			*layoutElements = ReadLayoutElements(json["InputLayout"]);
			if (layoutElements->size() > 0) {
				info.GraphicsPipeline.InputLayout.LayoutElements = layoutElements->data();
				info.GraphicsPipeline.InputLayout.NumElements = layoutElements->size();
			}
		}

		if (json.contains("ResourceLayout")) {
			info.PSODesc.ResourceLayout = ReadResourceLayout(json["ResourceLayout"],
				variables,
				immutableSamplers,
				strings);
		}

		return info;
	}

	DG::IShader* PipelineLoader::LoadShader(
		const nlohmann::json& shaderConfig,
		const std::string& path,
		const ShaderPreprocessorConfig* config) {
		DG::ShaderCreateInfo info;
		info.Desc.ShaderType = ReadShaderType(shaderConfig.value("ShaderType", ""));
		std::string name = shaderConfig.value("Name", "Unnammed Shader");
		info.Desc.Name = name.c_str();
		
		std::string entryPoint = shaderConfig.value("EntryPoint", "main");
		info.EntryPoint = entryPoint.c_str();

		std::string source = shaderConfig.value("Source", "");

		std::cout << "Loading " << source << "..." << std::endl;

		ShaderPreprocessorOutput output;
		mShaderLoader.Load(path + "/" + source, &output, config);

		info.Source = output.mContent.c_str();
		info.SourceLanguage = DG::SHADER_SOURCE_LANGUAGE_HLSL;

		DG::IShader* shader = nullptr;
		mManager->GetParent()->GetDevice()->CreateShader(info, &shader);
		return shader;
	}

	DG::IShader* PipelineLoader::LoadShader(DG::SHADER_TYPE shaderType,
		const std::string& path,
		const std::string& name,
		const std::string& entryPoint,
		const ShaderPreprocessorConfig* config) {
		DG::ShaderCreateInfo info;
		info.Desc.ShaderType = shaderType;
		info.Desc.Name = name.c_str();
		info.EntryPoint = entryPoint.c_str();

		std::cout << "Loading " << path << "..." << std::endl;

		ShaderPreprocessorOutput output;
		mShaderLoader.Load(path, &output, config);

		info.Source = output.mContent.c_str();
		info.SourceLanguage = DG::SHADER_SOURCE_LANGUAGE_HLSL;

		DG::IShader* shader = nullptr;
		mManager->GetParent()->GetDevice()->CreateShader(info, &shader);

		return shader;
	}

	ResourceCache<PipelineResource>::~ResourceCache() {
		Clear();
	}

	PipelineResource::~PipelineResource() {
		mState->Release();
	}

	Resource* ResourceCache<PipelineResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<PipelineResource>*>(params);
		auto src = params_cast->mSource;
		auto overrides = &params_cast->mOverrides;

		auto it = mCachedResources.find(src);
		if (it != mCachedResources.end()) {
			return it->second;
		}

		PipelineResource* resource = new PipelineResource(mManager);
		mLoader.Load(src, resource, overrides);
		resource->mSource = params_cast->mSource;
		mCachedResources[src] = resource;
		return resource;
	}

	Resource* ResourceCache<PipelineResource>::DeferredLoad(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<PipelineResource>*>(params);
		auto src = params_cast->mSource;

		auto it = mCachedResources.find(src);
		if (it != mCachedResources.end()) {
			return it->second;
		}

		PipelineResource* resource = new PipelineResource(mManager);
		mCachedResources[src] = resource;
		// Load this later
		mDefferedResources.push_back(std::make_pair(resource, *params_cast));
		return resource;
	}

	void ResourceCache<PipelineResource>::ProcessDeferred() {
		for (auto& resource : mDefferedResources) {
			mLoader.Load(resource.second.mSource, resource.first, &resource.second.mOverrides);
			resource.first->mSource = resource.second.mSource;
		}

		mDefferedResources.clear();
	}

	void ResourceCache<PipelineResource>::Add(Resource* resource, const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<PipelineResource>*>(params);
		auto src = params_cast->mSource;

		auto pipeline = resource->ToPipeline();

		auto it = mCachedResources.find(src);
		if (it != mCachedResources.end()) {
			if (it->second != pipeline)
				Unload(it->second);
			else
				return;
		}
		mCachedResources[src] = pipeline;
	}

	void ResourceCache<PipelineResource>::Unload(Resource* resource) {
		auto pipeline = resource->ToPipeline();

		auto it = mCachedResources.find(pipeline->GetSource());
		if (it != mCachedResources.end()) {
			if (it->second == pipeline) {
				mCachedResources.erase(it);
			}
		}

		delete resource;
	}

	void ResourceCache<PipelineResource>::Clear() {
		for (auto& item : mCachedResources) {
			item.second->ResetRefCount();
			delete item.second;
		}
	}
}