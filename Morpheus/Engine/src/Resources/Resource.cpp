#include <Engine/Resources/Resource.hpp>

#include <fstream>

namespace Morpheus {

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
}