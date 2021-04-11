#pragma once

#include <Engine/Engine.hpp>
#include <Engine/Scene.hpp>
#include <Engine/Im3d.hpp>

#include <memory>

#include "imgui.h"

#include <im3d.h>

namespace Morpheus {
	struct Editor2DParams {
		Engine* mEngine;
		ResourceManager* mResourceManager;
		ImFont* mEditorFont = nullptr;

		inline Editor2DParams(Engine* engine) : mEngine(engine) {
			mResourceManager = engine->GetResourceManager();
		}
	};

	class Editor2D {
	private:
		ImFont* mInternalFont = nullptr;
		std::unique_ptr<Im3dRenderer> mIm3dRenderer;
		std::unique_ptr<Im3dGlobalsBuffer> mIm3dGlobalsBuffer;
		Im3d::Context mContext;

		float mTranslation[3];

	public:
		void Initialize(const Editor2DParams& params);
		void Update(Engine* engine, Scene* scene, float dt);
		void Render(Engine* engine, Scene* scene, DG::IDeviceContext* context);
		void RenderUI(Engine* engine, Scene* scene);

		inline Editor2D() {
		}

		inline Editor2D(const Editor2DParams& params) {
			Initialize(params);
		}

		static bool GizmoTranslation(Im3d::Id id, float translation[3], bool local, Im3d::Context& context);
	};
}