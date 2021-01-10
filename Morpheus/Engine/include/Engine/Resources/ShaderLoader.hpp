#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include <nlohmann/json.hpp>
#include <Engine/Resources/Resource.hpp>

#include "RenderDevice.h"
#include "DeviceContext.h"

namespace DG = Diligent;

// The contents of this function are generated with CMake, and located in a
// generated shader_rc.cpp.
void makeSourceMap(std::unordered_map<std::string, const char*>* map);

namespace Morpheus {
	struct ShaderPreprocessorConfig {
		std::unordered_map<std::string, std::string> mDefines;

		std::string Stringify(const ShaderPreprocessorConfig* overrides) const;
	};

	class IShaderSourceLoader {
	public:
		virtual bool TryFind(const std::string& source, std::string* contents) = 0;
		virtual bool TryLoadJson(const std::string& source, nlohmann::json& result) = 0;
	};

	struct ShaderPreprocessorOutput {
		std::vector<std::string> mSources;
		std::string mContent;
	};
	
	class ShaderPreprocessor {
	private:
		IShaderSourceLoader* mSourceInterface;

		void Load(const std::string& source,
			const std::string& path,
			const ShaderPreprocessorConfig* defaults,
			const ShaderPreprocessorConfig* overrides,
			std::stringstream* streamOut,
			ShaderPreprocessorOutput* output, 
			std::set<std::string>* alreadyVisited);

	public:
		inline ShaderPreprocessor(IShaderSourceLoader* loader) : mSourceInterface(loader) {
		}

		void Load(const std::string& source, ShaderPreprocessorOutput* output, 
			const ShaderPreprocessorConfig* defaults, 
			const ShaderPreprocessorConfig* overrides = nullptr);
	};

	class ShaderLoader : public IShaderSourceLoader {
	private:
		ResourceManager* mManager;
		ShaderPreprocessor mPreprocessor;
		std::unordered_map<std::string, const char*> mInternalShaders;

	public:
		ShaderLoader(ResourceManager* manager);

		bool TryFind(const std::string& source, std::string* contents) override;
		bool TryLoadJson(const std::string& source, nlohmann::json& result) override;

		void Load(const std::string& source,
			ShaderPreprocessorOutput* output,
			const ShaderPreprocessorConfig* overrides = nullptr);
	};
}