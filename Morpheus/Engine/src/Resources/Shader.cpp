#include <Engine/Resources/Shader.hpp>
#include <Engine/Resources/ShaderPreprocessor.hpp>

namespace Morpheus {
	DG::IShader* RawShader::ToDiligent(DG::IRenderDevice* device) {
		DG::IShader* shader = nullptr;
		mCreateInfo.Source = mShaderSource.c_str();
		mCreateInfo.EntryPoint = mEntryPoint.c_str();
		mCreateInfo.Desc.Name = mName.c_str();
		device->CreateShader(mCreateInfo, &shader);
		return shader;
	}

	DG::IShader* CompileShader(DG::IRenderDevice* device, 
		const ShaderPreprocessorOutput& preprocessorOutput,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint) {
		RawShader raw(preprocessorOutput, type, name, entryPoint);
		return raw.ToDiligent(device);
	}

	DG::IShader* CompileEmbeddedShader(DG::IRenderDevice* device,
		const std::string& source,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint,
		const ShaderPreprocessorConfig* config,
		IVirtualFileSystem* fileLoader) {

		ShaderPreprocessor preprocessor;
		ShaderPreprocessorOutput output;
		ShaderPreprocessorConfig defaultConfig;
		preprocessor.Load(source, fileLoader, &output, &defaultConfig, config);
		RawShader raw(output, type, name, entryPoint);
		return raw.ToDiligent(device);
	}

	template <typename T>
	Future<T> LoadShaderTemplated(DG::IRenderDevice* device, 
		const LoadParams<RawShader>& shader,
		IVirtualFileSystem* fileSystem,
		ShaderPreprocessorConfig* defaults) {

		Promise<RawShader> raw;
		Promise<T> result;

		FunctionPrototype<Promise<RawShader>> rawPrototype(
			[params = shader, device, fileSystem, defaults]
				(const TaskParams& e, Promise<RawShader> result) mutable {
			ShaderPreprocessorConfig nothing;
			if (!defaults)
				defaults = &nothing;

			ShaderPreprocessorOutput output;
			ShaderPreprocessor::Load(params.mSource, fileSystem, &output, defaults, &params.mOverrides);
			result = RawShader(output, params.mShaderType, params.mName, params.mEntryPoint);
		});

		FunctionPrototype<UniqueFuture<RawShader>, Promise<T>> diligentPrototype(
			[params = shader, device, fileSystem, defaults]
				(const TaskParams& e, UniqueFuture<RawShader> raw, Promise<T> output) {
			DG::IShader* shader = raw.Get().ToDiligent(device);
			output = shader;
			if constexpr (std::is_same_v<T, Handle<DG::IShader>>) {
				shader->Release();
			}
		});

		rawPrototype(raw).SetName("Load Raw Shader");
		diligentPrototype(raw, result).SetName("Compile Shader").OnlyThread(THREAD_MAIN);
		
		return result;
	}

	Future<Handle<DG::IShader>> LoadShaderHandle(DG::IRenderDevice* device, 
		const LoadParams<RawShader>& shader,
		IVirtualFileSystem* fileSystem,
		ShaderPreprocessorConfig* defaults) {
		return LoadShaderTemplated<Handle<DG::IShader>>(device, shader, fileSystem, defaults);
	}

	// Loads resource and adds resulting task to the specified barrier
	Future<DG::IShader*> LoadShader(DG::IRenderDevice* device, 
		const LoadParams<RawShader>& params,
		IVirtualFileSystem* system,
		ShaderPreprocessorConfig* defaults) {
		return LoadShaderTemplated<DG::IShader*>(device, params, system, defaults);
	}
}