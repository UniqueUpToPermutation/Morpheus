#include <Engine/Resources/Shader.hpp>
#include <Engine/Resources/ShaderPreprocessor.hpp>

namespace Morpheus {
	DG::IShader* RawShader::SpawnOnGPU(DG::IRenderDevice* device) {
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
		return raw.SpawnOnGPU(device);
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
		return raw.SpawnOnGPU(device);
	}

	// Loads resource and adds resulting task to the specified barrier
	ResourceTask<DG::IShader*> LoadShader(DG::IRenderDevice* device, 
		const LoadParams<RawShader>& params,
		IVirtualFileSystem* system,
		ShaderPreprocessorConfig* defaults) {

		struct Data {
			RawShader mRawShader;
		};

		Promise<DG::IShader*> promise;
		Future<DG::IShader*> future(promise);

		Task task([params = LoadParams<RawShader>(params), device, system, 
			defaults, data = Data(), promise = std::move(promise)](const TaskParams& e) mutable {
			if (e.mTask->BeginSubTask()) {
				ShaderPreprocessorConfig nothing;
				if (!defaults)
					defaults = &nothing;

				ShaderPreprocessorOutput output;
				ShaderPreprocessor::Load(params.mSource, system, &output, defaults, &params.mOverrides);
				data.mRawShader = RawShader(output, params.mShaderType, params.mName, params.mEntryPoint);

				e.mTask->EndSubTask();
				if (e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN))
					return TaskResult::REQUEST_THREAD_SWITCH;
			}

			if (e.mTask->BeginSubTask()) {
				DG::IShader* shader = data.mRawShader.SpawnOnGPU(device);
				promise.Set(shader, e.mQueue);
				e.mTask->EndSubTask();
			}

			return TaskResult::FINISHED;
		}, 
		std::string("Load Shader ") + params.mSource,
		TaskType::FILE_IO);

		ResourceTask<DG::IShader*> resTask;
		resTask.mFuture = std::move(future);
		resTask.mTask = std::move(task);
		
		return resTask;
	}
}