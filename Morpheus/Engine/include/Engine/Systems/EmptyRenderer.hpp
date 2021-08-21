#pragma once

#include <Engine/Graphics.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Renderer.hpp>
#include <Engine/GeometryStructures.hpp>

namespace Morpheus {
	class EmptyRenderer : public ISystem, 
		public IRenderer, 
		public IVertexFormatProvider {
	private:
		RealtimeGraphics* mGraphics;
		VertexLayout mDefaultLayout = VertexLayout::PositionUVNormalTangent();
	
	public:
		inline EmptyRenderer(RealtimeGraphics& graphics) : mGraphics(&graphics) {
		}

		inline RealtimeGraphics* GetGraphics() {
			return mGraphics;
		}

		std::unique_ptr<Task> Startup(SystemCollection& collection) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;

		const VertexLayout& GetStaticMeshLayout() const override;
		
		MaterialId CreateUnmanagedMaterial(const MaterialDesc& desc) override;

		void AddMaterialRef(MaterialId id) override;
		void ReleaseMaterial(MaterialId id) override;

		GraphicsCapabilityConfig GetCapabilityConfig() const;
	};
}