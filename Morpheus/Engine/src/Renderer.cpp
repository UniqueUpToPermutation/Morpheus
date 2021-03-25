#include <Engine/Renderer.hpp>

namespace Morpheus {
	void IRenderer::Render(Scene* scene, EntityNode cameraNode, DG::ISwapChain* swapChain) {
		RenderPassTargets targets;
		targets.mColorOutputs.push_back(swapChain->GetCurrentBackBufferRTV());
		targets.mDepthOutput = swapChain->GetDepthBufferDSV();

		Render(scene, cameraNode, targets);
	}
}