#pragma once

#include <unordered_map>
#include <filesystem>
#include <nlohmann/json.hpp>

struct PathHasher {
	std::size_t operator() (const std::filesystem::path& path) const {
		return std::filesystem::hash_value(path);
	}
};

typedef std::unordered_map<std::filesystem::path, const char*, PathHasher> file_map_t;

// The contents of this function are generated with CMake, and located in a
// generated shader_rc.cpp.
void MakeSourceMap(file_map_t* map);

namespace Morpheus {

	typedef std::function<void(file_map_t*)> embedded_file_loader_t;
	class IVirtualFileSystem {
	public:
		virtual ~IVirtualFileSystem() {
		}

		virtual bool Exists(const std::filesystem::path& source) const = 0;
		virtual bool TryFind(const std::filesystem::path& source, std::string* contents) const = 0;
		virtual bool TryLoadJson(const std::filesystem::path& source, nlohmann::json& result) const = 0;
	};

	class EmbeddedFileLoader : public IVirtualFileSystem {
	private:
		file_map_t mInternalShaders;

		static EmbeddedFileLoader* mGlobalInstance;

	public:
		void Add(const embedded_file_loader_t& factory);

		inline EmbeddedFileLoader() {
			Add(&MakeSourceMap);
			mGlobalInstance = this;
		}

		bool Exists(const std::filesystem::path& source) const override;
		bool TryFind(const std::filesystem::path& source, std::string* contents) const override;
		bool TryLoadJson(const std::filesystem::path& source, nlohmann::json& result) const override;
	
		static inline EmbeddedFileLoader* GetGlobalInstance() {
			return mGlobalInstance;
		}
	};
}