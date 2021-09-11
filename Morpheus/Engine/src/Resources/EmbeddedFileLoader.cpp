#include <Engine/Resources/EmbeddedFileLoader.hpp>
#include <nlohmann/json.hpp>

namespace Morpheus {
	EmbeddedFileLoader* EmbeddedFileLoader::mGlobalInstance;

	bool EmbeddedFileLoader::Exists(const std::filesystem::path& source) const {
		auto searchSource = std::filesystem::relative(source, "");

		auto it = mInternalShaders.find(searchSource);
		if (it != mInternalShaders.end()) {
			return true;
		} else {
			return false;
		}
	}

	bool EmbeddedFileLoader::TryFind(const std::filesystem::path& source, std::string* contents) const {
		auto searchSource = std::filesystem::relative(source, "");
		
		// Search internal shaders first
		auto it = mInternalShaders.find(searchSource);
		if (it != mInternalShaders.end()) {
			*contents = it->second;
			return true;
		}
		return false;
	}

	bool EmbeddedFileLoader::TryLoadJson(const std::filesystem::path& source, nlohmann::json& result) const {
		auto searchSource = std::filesystem::relative(source, "");
		
		auto it = mInternalShaders.find(searchSource);
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