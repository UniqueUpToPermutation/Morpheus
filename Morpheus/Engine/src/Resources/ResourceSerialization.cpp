#include <Engine/Resources/ResourceSerialization.hpp>
#include <Engine/Resources/TextureResource.hpp>
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

	void Load(cereal::PortableBinaryInputArchive& ar, RawTexture* texture) {
		DG::TextureDesc desc;
		std::vector<TextureSubResDataDesc> subs;
		float intensity;
		std::vector<uint8_t> data;

		ar(desc);
		ar(subs);
		ar(intensity);

		auto valueType = GetComponentType(desc.Format);

		size_t arrayLength;
		ar(arrayLength);

		size_t byteLength = GetTypeSize(valueType) * arrayLength;
		data.resize(byteLength);

		switch (valueType) {
			case DG::VT_FLOAT32:
			{
				DG::Float32* ptr = (DG::Float32*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT32:
			{
				DG::Int32* ptr = (DG::Int32*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT32:
			{
				DG::Uint32* ptr = (DG::Uint32*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_FLOAT16:
			{
				DG::Uint16* ptr = (DG::Uint16*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT16:
			{
				DG::Int16* ptr = (DG::Int16*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT16:
			{
				DG::Uint16* ptr = (DG::Uint16*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT8:
			{
				DG::Uint8* ptr = (DG::Uint8*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT8:
			{
				DG::Int8* ptr = (DG::Int8*)(&data[0]);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			default:
				throw std::runtime_error("Invalid value type!");
		}

		texture->Set(desc, std::move(data), subs);
		texture->SetIntensity(intensity);
	}

	void Save(cereal::PortableBinaryOutputArchive& ar, const RawTexture* texture) {
		ar(texture->GetDesc());
		ar(texture->GetSubDataDescs());
		ar(texture->GetIntensity());

		auto valueType = GetComponentType(texture->GetDesc().Format);

		size_t arrayLength;

		switch (valueType) {
			case DG::VT_FLOAT32:
			{
				const float* ptr = (float*)(&texture->GetData()[0]);
				arrayLength = texture->GetData().size() / sizeof(float);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT32:
			{
				const DG::Int32* ptr = (const DG::Int32*)(&texture->GetData()[0]);
				arrayLength = texture->GetData().size() / sizeof(DG::Int32);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT32:
			{
				const DG::Uint32* ptr = (const DG::Uint32*)(&texture->GetData()[0]);
				arrayLength = texture->GetData().size() / sizeof(DG::Uint32);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_FLOAT16:
			{
				const DG::Uint16* ptr = (const DG::Uint16*)(&texture->GetData()[0]);
				arrayLength = texture->GetData().size() / sizeof(DG::Uint16);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT16:
			{
				const DG::Int16* ptr = (const DG::Int16*)(&texture->GetData()[0]);
				arrayLength = texture->GetData().size() / sizeof(DG::Int16);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT16:
			{
				const DG::Uint16* ptr = (const DG::Uint16*)(&texture->GetData()[0]);
				arrayLength = texture->GetData().size() / sizeof(DG::Uint16);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_UINT8:
			{
				const DG::Uint8* ptr = (const DG::Uint8*)&texture->GetData()[0];
				arrayLength = texture->GetData().size() / sizeof(DG::Uint8);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			case DG::VT_INT8:
			{
				const DG::Int8* ptr = (const DG::Int8*)&texture->GetData()[0];
				arrayLength = texture->GetData().size() / sizeof(DG::Int8);
				ar(arrayLength);
				ar(cereal::binary_data(ptr, arrayLength));
				break;
			}
			default:
				throw std::runtime_error("Invalid value type!");
		}
	}

	void Load(cereal::PortableBinaryInputArchive& ar, RawGeometry* geometry) {

	}

	void Save(cereal::PortableBinaryOutputArchive& ar, const RawGeometry* geometry) {

	}
}