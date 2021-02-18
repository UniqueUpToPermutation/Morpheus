#pragma once

#include <unordered_map>
#include <nlohmann/json.hpp>

// The contents of this function are generated with CMake, and located in a
// generated shader_rc.cpp.
void MakeSourceMap(std::unordered_map<std::string, const char*>* map);

namespace Morpheus {

	class IVirtualFileSystem {
	public:
		virtual ~IVirtualFileSystem() {
		}

		virtual bool Exists(const std::string& source) const = 0;
		virtual bool TryFind(const std::string& source, std::string* contents) const = 0;
		virtual bool TryLoadJson(const std::string& source, nlohmann::json& result) const = 0;
	};

	class EmbeddedFileLoader : public IVirtualFileSystem {
	private:
		std::unordered_map<std::string, const char*> mInternalShaders;

		static EmbeddedFileLoader* mGlobalInstance;

	public:
		inline EmbeddedFileLoader() {
			MakeSourceMap(&mInternalShaders);
			mGlobalInstance = this;
		}

		bool Exists(const std::string& source) const override;
		bool TryFind(const std::string& source, std::string* contents) const override;
		bool TryLoadJson(const std::string& source, nlohmann::json& result) const override;
	
		static inline EmbeddedFileLoader* GetGlobalInstance() {
			return mGlobalInstance;
		}
	};
}