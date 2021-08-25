#include <Engine/Resources/EmbeddedFileLoader.hpp>
#include <nlohmann/json.hpp>

namespace Morpheus {
	EmbeddedFileLoader* EmbeddedFileLoader::mGlobalInstance;

	bool EmbeddedFileLoader::Exists(const std::string& source) const {
		auto it = mInternalShaders.find(source);
		if (it != mInternalShaders.end()) {
			return true;
		} else {
			return false;
		}
	}

	bool EmbeddedFileLoader::TryFind(const std::string& source, std::string* contents) const {
		// Search internal shaders first
		auto it = mInternalShaders.find(source);
		if (it != mInternalShaders.end()) {
			*contents = it->second;
			return true;
		}
		return false;
	}

	bool EmbeddedFileLoader::TryLoadJson(const std::string& source, nlohmann::json& result) const {
		auto it = mInternalShaders.find(source);
		if (it != mInternalShaders.end()) {
			result = nlohmann::json::parse(it->second);
			return true;
		} else {
			return false;
		}
	}

	void EmbeddedFileLoader::Add(const embedded_file_loader_t& factory) {
		factory(&mInternalShaders);
	}
}