#pragma once

#include <Engine/Resources/ShaderPreprocessor.hpp>
#include <string>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {
	class RawShader;

	template <>
	struct LoadParams<RawShader> {
		std::string mSource;
		bool bCache = false;
		const ShaderPreprocessorConfig mOverrides;
		std::string mName;
		std::string mEntryPoint;
		DG::SHADER_TYPE mShaderType;

		LoadParams(const std::string& source,
			DG::SHADER_TYPE type,
			const std::string& name,
			const ShaderPreprocessorConfig& overrides = ShaderPreprocessorConfig(),
			const std::string& entryPoint = "main",
			bool cache = false) :
			mSource(source),
			mShaderType(type),
			mName(name),
			mOverrides(overrides),
			mEntryPoint(entryPoint),
			bCache(cache) {
		}
	};

	class RawShader {
	private:
		std::string mShaderSource;
		std::string mEntryPoint;
		std::string mName;
		DG::ShaderCreateInfo mCreateInfo;

	public:
		inline RawShader() {
		}

		inline RawShader(const ShaderPreprocessorOutput& preprocessorOutput,
			DG::SHADER_TYPE type, const std::string& name, const std::string& entryPoint) :
			mShaderSource(preprocessorOutput.mContent),
			mName(name),
			mEntryPoint(entryPoint) {
			mCreateInfo.Desc.ShaderType = type;
			mCreateInfo.SourceLanguage = DG::SHADER_SOURCE_LANGUAGE_HLSL;
		}

		DG::IShader* ToDiligent(DG::IRenderDevice* device);
	};

	Future<DG::IShader*> LoadShader(DG::IRenderDevice* device, 
		const LoadParams<RawShader>& shader,
		IVirtualFileSystem* fileSystem = EmbeddedFileLoader::GetGlobalInstance(),
		ShaderPreprocessorConfig* defaults = nullptr);

	Future<Handle<DG::IShader>> LoadShaderHandle(DG::IRenderDevice* device, 
		const LoadParams<RawShader>& shader,
		IVirtualFileSystem* fileSystem = EmbeddedFileLoader::GetGlobalInstance(),
		ShaderPreprocessorConfig* defaults = nullptr);

	DG::IShader* CompileShader(DG::IRenderDevice* device, 
		const ShaderPreprocessorOutput& preprocessorOutput,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint);

	DG::IShader* CompileEmbeddedShader(DG::IRenderDevice* device,
		const std::string& source,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint,
		const ShaderPreprocessorConfig* config = nullptr,
		IVirtualFileSystem* fileLoader = EmbeddedFileLoader::GetGlobalInstance());

	inline DG::IShader* CompileEmbeddedShader(DG::IRenderDevice* device,
		const LoadParams<RawShader>& params,
		IVirtualFileSystem* fileLoader = EmbeddedFileLoader::GetGlobalInstance()) {
		return CompileEmbeddedShader(device, params.mSource,
			params.mShaderType,
			params.mName,
			params.mEntryPoint,
			&params.mOverrides,
			fileLoader);
	}
}