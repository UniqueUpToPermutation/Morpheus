#include <Engine/Resources/ResourceSerialization.hpp>
#include <Engine/Resources/RawTexture.hpp>
#include <Engine/Resources/RawGeometry.hpp>
#include <Engine/Resources/ResourceData.hpp>

#include <cereal/types/vector.hpp>

namespace Diligent {
	template <class Archive>
	void serialize(Archive& archive,
		DG::TextureDesc& m)
	{
		archive(m.ArraySize);
		archive(m.BindFlags);
		archive(m.ClearValue.Color);
		archive(m.ClearValue.DepthStencil.Depth);
		archive(m.ClearValue.DepthStencil.Stencil);
		archive(m.ClearValue.Format);
		archive(m.CommandQueueMask);
		archive(m.CPUAccessFlags);
		archive(m.Depth);
		archive(m.Format);
		archive(m.Height);
		archive(m.MipLevels);
		archive(m.MiscFlags);
		archive(m.SampleCount);
		archive(m.Type);
		archive(m.Usage);
		archive(m.Width);
	}

	template <class Archive>
	void serialize(Archive& archive,
		DG::BufferDesc& m) {
		archive(m.BindFlags);
		archive(m.CommandQueueMask);
		archive(m.CPUAccessFlags);
		archive(m.ElementByteStride);
		archive(m.Mode);
		archive(m.uiSizeInBytes);
		archive(m.Usage);
	}

	template <class Archive>
	void serialize(Archive& archive,
		DG::DrawAttribs& m) {
		archive(m.FirstInstanceLocation);
		archive(m.Flags);
		archive(m.NumInstances);
		archive(m.NumVertices);
		archive(m.StartVertexLocation);
	}

	template <class Archive>
	void serialize(Archive& archive,
		DG::DrawIndexedAttribs& m) {
		archive(m.BaseVertex);
		archive(m.FirstIndexLocation);
		archive(m.FirstInstanceLocation);
		archive(m.Flags);
		archive(m.IndexType);
		archive(m.NumIndices);
		archive(m.NumInstances);
	}

	template <class Archive>
	void serialize(Archive& archive,
		DG::LayoutElement& m) {
		archive(m.BufferSlot);
		archive(m.Frequency);
		archive(m.InputIndex);
		archive(m.InstanceDataStepRate);
		archive(m.IsNormalized);
		archive(m.NumComponents);
		archive(m.RelativeOffset);
		archive(m.Stride);
		archive(m.ValueType);
	}

	template <class Archive>
	void serialize(Archive& archive,
		DG::float2& m) {
		archive(m.x);
		archive(m.y);
	}

	template <class Archive>
	void serialize(Archive& archive,
		DG::float3& m) {
		archive(m.x);
		archive(m.y);
		archive(m.z);
	}

	template <class Archive>
	void serialize(Archive& archive,
		DG::float4& m) {
		archive(m.x);
		archive(m.y);
		archive(m.z);
		archive(m.w);
	}
}

namespace Morpheus {
	template<class Archive>
	void serialize(Archive& archive,
		TextureSubResDataDesc& subData)
	{
		archive(subData.mDepthStride);
		archive(subData.mSrcOffset);
		archive(subData.mStride);
	}

	template<class Archive>
	void serialize(Archive& archive,
		VertexLayout& layout) {
		archive(layout.mElements);
		archive(layout.mBitangent);
		archive(layout.mNormal);
		archive(layout.mPosition);
		archive(layout.mStride);
		archive(layout.mTangent);
		archive(layout.mUV);
	}

	template<class Archive>
	void serialize(Archive& archive,
		BoundingBox& box) {
		archive(box.mLower);
		archive(box.mUpper);
	}

	bool IsLittleEndian() {
		int n = 1;
		// little endian if true
		if(*(char *)&n == 1) 
			return true;
		else 
			return false;
	}

	bool IsBigEndian() {
		return !IsLittleEndian();
	}

	void LoadBinaryData(cereal::PortableBinaryInputArchive& ar, 
		DG::VALUE_TYPE valueType, std::vector<uint8_t>* data) {
		size_t arrayLength;
		ar(arrayLength);

		size_t byteLength = GetTypeSize(valueType) * arrayLength;
		data->resize(byteLength);

		switch (valueType) {
			case DG::VT_FLOAT32:
			{
				DG::Float32* ptr = (DG::Float32*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT32:
			{
				DG::Int32* ptr = (DG::Int32*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT32:
			{
				DG::Uint32* ptr = (DG::Uint32*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_FLOAT16:
			{
				DG::Uint16* ptr = (DG::Uint16*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT16:
			{
				DG::Int16* ptr = (DG::Int16*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT16:
			{
				DG::Uint16* ptr = (DG::Uint16*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT8:
			{
				DG::Uint8* ptr = (DG::Uint8*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT8:
			{
				DG::Int8* ptr = (DG::Int8*)(data->data());
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			default:
				throw std::runtime_error("Invalid value type!");
		}
	}

	void SaveBinaryData(cereal::PortableBinaryOutputArchive& ar, 
		DG::VALUE_TYPE valueType, const std::vector<uint8_t>* data) {
		size_t arrayLength;

		switch (valueType) {
			case DG::VT_FLOAT32:
			{
				const float* ptr = (float*)(data->data());
				arrayLength = data->size() / sizeof(float);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT32:
			{
				const DG::Int32* ptr = (const DG::Int32*)(data->data());
				arrayLength = data->size() / sizeof(DG::Int32);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT32:
			{
				const DG::Uint32* ptr = (const DG::Uint32*)(data->data());
				arrayLength = data->size() / sizeof(DG::Uint32);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_FLOAT16:
			{
				const DG::Uint16* ptr = (const DG::Uint16*)(data->data());
				arrayLength = data->size() / sizeof(DG::Uint16);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT16:
			{
				const DG::Int16* ptr = (const DG::Int16*)(data->data());
				arrayLength = data->size() / sizeof(DG::Int16);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT16:
			{
				const DG::Uint16* ptr = (const DG::Uint16*)(data->data());
				arrayLength = data->size() / sizeof(DG::Uint16);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT8:
			{
				const DG::Uint8* ptr = (const DG::Uint8*)(data->data());
				arrayLength = data->size() / sizeof(DG::Uint8);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT8:
			{
				const DG::Int8* ptr = (const DG::Int8*)(data->data());
				arrayLength = data->size() / sizeof(DG::Int8);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			default:
				throw std::runtime_error("Invalid value type!");
		}
	}

	void Load(cereal::PortableBinaryInputArchive& ar, RawTexture* texture) {
		DG::TextureDesc desc;
		std::vector<TextureSubResDataDesc> subs;
		float intensity;
		std::vector<uint8_t> data;

		uint version;

		ar(version);
		ar(desc);
		ar(subs);
		ar(intensity);

		auto valueType = GetComponentType(desc.Format);

		LoadBinaryData(ar, valueType, &data);

		texture->Set(desc, std::move(data), subs);
		texture->SetIntensity(intensity);
	}

	void Save(cereal::PortableBinaryOutputArchive& ar, const RawTexture* texture) {
		uint version = TEXTURE_ARCHIVE_VERSION;

		ar(version);
		ar(texture->GetDesc());
		ar(texture->GetSubDataDescs());
		ar(texture->GetIntensity());

		auto valueType = GetComponentType(texture->GetDesc().Format);

		SaveBinaryData(ar, valueType, &texture->GetData());
	}

	void Load(cereal::PortableBinaryInputArchive& ar, RawGeometry* geometry) {
		uint version;
		bool bHasIndexBuffer;
		BoundingBox aabb;
		DG::BufferDesc vertexDesc;
		DG::BufferDesc indexDesc;
		DG::DrawAttribs drawAttribs;
		DG::DrawIndexedAttribs indexedDrawAttribs;
		VertexLayout layout;
		std::vector<uint8_t> vertexData;
		std::vector<uint8_t> indexData;

		ar(version);
		ar(bHasIndexBuffer);
		ar(aabb);
		ar(vertexDesc);
		ar(indexDesc);
		ar(drawAttribs);
		ar(indexedDrawAttribs);
		ar(layout);

		DG::VALUE_TYPE vertexType = DG::VT_NUM_TYPES;
		DG::VALUE_TYPE indexType = indexedDrawAttribs.IndexType;

 		for (const auto& element : layout.mElements) {
			if (element.Frequency == DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX) {
				if (vertexType == DG::VT_NUM_TYPES) {
					vertexType = element.ValueType;
				} else {
					if (vertexType != element.ValueType) {
						throw std::runtime_error("To save to archive, all per vertex element must have the same type");
					}
				}
			}
		}

		LoadBinaryData(ar, vertexType, &vertexData);

		if (bHasIndexBuffer)
			LoadBinaryData(ar, indexType, &indexData);
	}

	void Save(cereal::PortableBinaryOutputArchive& ar, const RawGeometry* geometry) {

		uint version = GEOMETRY_ARCHIVE_VERSION;
		ar(version);
		ar(geometry->HasIndexBuffer());
		ar(geometry->GetBoundingBox());
		ar(geometry->GetVertexDesc());
		ar(geometry->GetIndexDesc());
		ar(geometry->GetDrawAttribs());
		ar(geometry->GetIndexedDrawAttribs());
		ar(geometry->GetLayout());

		DG::VALUE_TYPE vertexType = DG::VT_NUM_TYPES;
		DG::VALUE_TYPE indexType = geometry->GetIndexedDrawAttribs().IndexType;

		const auto& layout = geometry->GetLayout();
 		for (const auto& element : layout.mElements) {
			if (element.Frequency == DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX) {
				if (vertexType == DG::VT_NUM_TYPES) {
					vertexType = element.ValueType;
				} else {
					if (vertexType != element.ValueType) {
						throw std::runtime_error("To save to archive, all per vertex element must have the same type");
					}
				}
			}
		}

		SaveBinaryData(ar, vertexType, &geometry->GetVertexData());

		if (geometry->HasIndexBuffer())
			SaveBinaryData(ar, indexType, &geometry->GetIndexData());
	}
}