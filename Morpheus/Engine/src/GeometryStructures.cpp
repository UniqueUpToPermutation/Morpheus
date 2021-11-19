#include <Engine/GeometryStructures.hpp>

namespace Morpheus {
	VertexLayout VertexLayout::PositionUVNormalTangent() {
		VertexLayout layout;
		layout.mPosition = 0;
		layout.mUVs = {1};
		layout.mNormal = 2;
		layout.mTangent = 3;

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 2, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(3, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

			DG::LayoutElement(4, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(6, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(7, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}

	VertexLayout VertexLayout::PositionUVNormal() {
		VertexLayout layout;
		layout.mPosition = 0;
		layout.mUVs = {1};
		layout.mNormal = 2;

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 2, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

			DG::LayoutElement(3, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(4, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(6, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}

	VertexLayout VertexLayout::PositionUVNormalTangentBitangent() {
		VertexLayout layout;
		layout.mPosition = 0;
		layout.mUVs = {1};
		layout.mNormal = 2;
		layout.mTangent = 3;
		layout.mBitangent = 4;

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 2, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(3, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(4, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

			DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(6, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(7, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(8, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}
}