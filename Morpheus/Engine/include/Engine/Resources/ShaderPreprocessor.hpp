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
		static void Load(const std::filesystem::path& source,
			IVirtualFileSystem* fileLoader,
			const ShaderPreprocessorConfig* defaults,
			const ShaderPreprocessorConfig* overrides,
			std::stringstream* streamOut,
			ShaderPreprocessorOutput* output, 
			std::unordered_set<std::filesystem::path, PathHasher>* alreadyVisited);

	public:
		static void Load(const std::filesystem::path& source,
			IVirtualFileSystem* fileLoader,
			ShaderPreprocessorOutput* output, 
			const ShaderPreprocessorConfig* defaults, 
			const ShaderPreprocessorConfig* overrides = nullptr);
	};
}