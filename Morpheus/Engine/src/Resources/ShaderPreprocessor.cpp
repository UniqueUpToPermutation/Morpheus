#include <Engine/Resources/ShaderPreprocessor.hpp>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>

namespace Morpheus {

	std::string ShaderPreprocessorConfig::Stringify(const ShaderPreprocessorConfig* overrides) const {
		std::stringstream ss;
	
		for (auto& it : mDefines) {
			ss << "#define " << it.first << " " << it.second << std::endl;
		}
		if (overrides) {
			for (auto& it : overrides->mDefines) {
				ss << "#undef " << it.first << std::endl;
				ss << "#define " << it.first << " " << it.second << std::endl;
			}
		}
		return ss.str();
	}

	void ShaderPreprocessor::Load(
		const std::filesystem::path& source,
		IVirtualFileSystem* fileLoader,
		const ShaderPreprocessorConfig* defaults,
		const ShaderPreprocessorConfig* overrides,
		std::stringstream* streamOut,
		ShaderPreprocessorOutput* output, 
		std::unordered_set<std::filesystem::path, PathHasher>* alreadyVisited) {

		if (alreadyVisited->find(source) != alreadyVisited->end()) {
			return;
		}

		alreadyVisited->emplace(source);

		std::string contents;
		if (!fileLoader->TryFind(source, &contents)) {
			std::cout << "Unable to find: " << source << std::endl;
			throw new std::runtime_error(std::string("Unable to find: ") + source.string());
		}

		output->mSources.emplace_back(source);

		// Write preprocessed string
		std::stringstream& ss = *streamOut;
		if (alreadyVisited->size() == 1) {
			ss << defaults->Stringify(overrides) << std::endl;
		}

		auto sourceStr = source.string();

		ss << std::endl << "#line 1 \"" << sourceStr << "\"" << std::endl; // Reset line numbers

		// Find all includes
		size_t include_pos = contents.find("#include");
		size_t last_include_pos = 0;
		size_t current_line = 0;
		while (include_pos != std::string::npos) {

			current_line += std::count(&contents[last_include_pos], &contents[include_pos], '\n');

			size_t endLineIndex = contents.find('\n', include_pos);
			bool bGlobalSearch = false;

			auto quotesIt = std::find(&contents[include_pos], &contents[endLineIndex], '\"');

			ss << contents.substr(last_include_pos, include_pos - last_include_pos);

			if (quotesIt == &contents[endLineIndex]) {
				std::cout << sourceStr << ": Warning: #include detected without include file!" << std::endl;
			}
			else {
				size_t endIndx;
				size_t startIndx;

				startIndx = quotesIt - &contents[0] + 1;
				endIndx = contents.find('\"', startIndx);
				bGlobalSearch = false;
				
				if (endIndx == std::string::npos) {
					std::cout << sourceStr << ": Warning: unmatched quote in #include!" << std::endl;
					return;
				}

				last_include_pos = endIndx + 1;

				std::filesystem::path includeSource = contents.substr(startIndx, endIndx - startIndx);
				std::filesystem::path nextPath = source.parent_path();

				if (bGlobalSearch) {
					Load(includeSource, fileLoader, defaults, overrides, streamOut,
						output, alreadyVisited);

				} else {
					includeSource = nextPath / includeSource;

					Load(includeSource, fileLoader, defaults, overrides, streamOut,
						output, alreadyVisited);
				}

				ss << std::endl << "\n#line " << current_line + 1 << " \"" << sourceStr << "\"" << std::endl; // Reset line numbers
			}

			include_pos = contents.find("#include", include_pos + 1);
		}

		ss << contents.substr(last_include_pos);
	}

	void ShaderPreprocessor::Load(
		const std::filesystem::path& source,
		IVirtualFileSystem* fileLoader,
		ShaderPreprocessorOutput* output,
		const ShaderPreprocessorConfig* defaults, 
		const ShaderPreprocessorConfig* overrides) {

		std::unordered_set<std::filesystem::path, PathHasher> alreadyVisited;
		std::stringstream streamOut;

		Load(source, fileLoader,
			defaults, overrides,
			&streamOut, output, &alreadyVisited);

		output->mContent = streamOut.str();
	}
}