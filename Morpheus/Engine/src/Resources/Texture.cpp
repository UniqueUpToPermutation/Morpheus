#include <Engine/Resources/Texture.hpp>

namespace Morpheus {

	template <typename R>
	ResourceTask<R> LoadTemplated(DG::IRenderDevice* device, const LoadParams<Texture>& params) {
		Promise<R> promise;
		Future<R> future(promise);

		struct Data {
			RawTexture mRaw;
		};

		Task task([params, device, promise = std::move(promise), data = Data()](const TaskParams& e) mutable {
			if (e.mTask->BeginSubTask()) {
				data.mRaw.Load(params);
				e.mTask->EndSubTask();
			}

			e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN);

			if (e.mTask->BeginSubTask()) {
				auto gpuTex = data.mRaw.SpawnOnGPU(device);
				Texture* texture = new Texture(gpuTex);
				
				promise.Set(texture, e.mQueue);

				if constexpr (std::is_same_v<R, Handle<Texture>>) {
					texture->Release();
				}

				e.mTask->EndSubTask();
			}
		}, 
		std::string("Load ") + params.mSource, 
		TaskType::FILE_IO);

		ResourceTask<R> resourceTask;
		resourceTask.mTask = std::move(task);
		resourceTask.mFuture = future;

		return resourceTask;
	}

	ResourceTask<Handle<Texture>> Texture::LoadHandle(DG::IRenderDevice* device, const LoadParams<Texture>& params) {
		return LoadTemplated<Handle<Texture>>(device, params);
	}

	ResourceTask<Texture*> Texture::Load(DG::IRenderDevice* device, const LoadParams<Texture>& params) {
		return LoadTemplated<Texture*>(device, params);
	}
}