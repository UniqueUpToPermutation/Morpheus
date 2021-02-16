#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include <nlohmann/json.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/EmbeddedFileLoader.hpp>

#include "RenderDevice.h"
#include "DeviceContext.h"

namespace DG = Diligent;

namespace Morpheus {
	struct ShaderPreprocessorConfig {
		std::unordered_map<std::string, std::string> mDefines;

		std::string Stringify(const ShaderPreprocessorConfig* overrides) const;
	};

	struct ShaderPreprocessorOutput {
		std::vector<std::string> mSources;
		std::string mContent;
	};
	
	class ShaderPreprocessor {
	private:
		void Load(const std::string& source,
			EmbeddedFileLoader* fileLoader,
			const std::string& path,
			const ShaderPreprocessorConfig* defaults,
			const ShaderPreprocessorConfig* overrides,
			std::stringstream* streamOut,
			ShaderPreprocessorOutput* output, 
			std::set<std::string>* alreadyVisited);

	public:
		void Load(const std::string& source,
			EmbeddedFileLoader* fileLoader,
			ShaderPreprocessorOutput* output, 
			const ShaderPreprocessorConfig* defaults, 
			const ShaderPreprocessorConfig* overrides = nullptr);
	};

	class ShaderLoader {
	private:
		ResourceManager* mManager;
		ShaderPreprocessor mPreprocessor;

	public:
		ShaderLoader(ResourceManager* manager);

		void Load(const std::string& source,
			EmbeddedFileLoader* fileLoader,
			ShaderPreprocessorOutput* output,
			const ShaderPreprocessorConfig* overrides = nullptr);
	};
}