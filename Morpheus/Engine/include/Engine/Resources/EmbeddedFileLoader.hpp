#pragma once

#include <unordered_map>

// The contents of this function are generated with CMake, and located in a
// generated shader_rc.cpp.
void MakeSourceMap(std::unordered_map<std::string, const char*>* map);

namespace Morpheus {
	class EmbeddedFileLoader {
	private:
		std::unordered_map<std::string, const char*> mInternalShaders;

	public:
		inline EmbeddedFileLoader() {
			MakeSourceMap(&mInternalShaders);
		}

		inline bool TryFind(const std::string& source, std::string* contents) {
			// Search internal shaders first
			auto it = mInternalShaders.find(source);
			if (it != mInternalShaders.end()) {
				*contents = it->second;
				return true;
			}
			return false;
		}

		inline bool TryLoadJson(const std::string& source, nlohmann::json& result) {
			auto it = mInternalShaders.find(source);
			if (it != mInternalShaders.end()) {
				result = nlohmann::json::parse(it->second);
				return true;
			} else {
				return false;
			}
		}
	};
}