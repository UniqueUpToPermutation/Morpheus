#include <Engine/Resources/Resource.hpp>

#include <Engine/Entity.hpp>

#include <fstream>

#include <cereal/archives/portable_binary.hpp>

using namespace entt;

namespace Morpheus {
	void IResource::Move(Device device, Context context) {
		auto barrier = MoveAsync(device, context);
		context.Flush();
		barrier.Evaluate();
	}

	void ReadBinaryFile(const std::string& source, std::vector<uint8_t>& out) {
		auto stream = std::ifstream(source, std::ios::binary | std::ios::ate);
	
		size_t data_size = 0;

		if (stream.is_open()) {
			std::streamsize size = stream.tellg();
			data_size = (size_t)(size + 1);
			stream.seekg(0, std::ios::beg);

			out.resize(data_size);
			if (!stream.read((char*)&out[0], size))
			{
				std::runtime_error("Could not read file");
			}

			stream.close();

			out[size] = 0;
		}
	}

	void IResource::RegisterMetaData() {
		meta<IResource>()
			.type("IResource"_hs);
	}

	void IResource::BinarySerializeReference(
		const std::filesystem::path& workingPath, 
		std::ostream& output) const {
		cereal::PortableBinaryOutputArchive arr(output);
		BinarySerializeReference(workingPath, arr);
	}

	void IResource::BinaryDeserializeReference(
		const std::filesystem::path& workingPath,
		std::istream& input) {
		cereal::PortableBinaryInputArchive arr(input);
		BinaryDeserializeReference(workingPath, arr);
	}

	void IResource::BinarySerializeToFile(const std::filesystem::path& output) const {
		std::ofstream f_(output, std::ios::binary);

		if (!f_.is_open()) {
			throw std::runtime_error(std::string("Failed to open ") + output.string());
		}

		BinarySerialize(f_);

		f_.close();
	}

	void IResource::BinaryDeserializeFromFile(const std::filesystem::path& input) {
		std::ifstream f_(input, std::ios::binary);

		if (!f_.is_open()) {
			throw std::runtime_error(std::string("Failed to open ") + input.string());
		}

		BinaryDeserialize(f_);

		f_.close();
	}

	UniversalIdentifier IResource::GetUniversalId() const {
		if (mFrame) {
			UniversalIdentifier id;
			id.mPath = mFrame->GetPath();
			id.mEntity = mEntity;
			return id;
		} else {
			UniversalIdentifier id;
			id.mPath = GetPath();
			return id;
		}
	}
}