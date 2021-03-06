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

	void ShaderPreprocessor::Load(const std::string& source,
		IVirtualFileSystem* fileLoader,
		const std::string& path,
		const ShaderPreprocessorConfig* defaults,
		const ShaderPreprocessorConfig* overrides,
		std::stringstream* streamOut,
		ShaderPreprocessorOutput* output, 
		std::set<std::string>* alreadyVisited) {

		if (alreadyVisited->find(source) != alreadyVisited->end()) {
			return;
		}

		alreadyVisited->emplace(source);

		std::string contents;
		if (!fileLoader->TryFind(source, &contents)) {
			std::cout << "Unable to find: " << source << std::endl;
			throw new std::runtime_error(std::string("Unable to find: ") + source);
		}

		output->mSources.emplace_back(source);

		// Write preprocessed string
		std::stringstream& ss = *streamOut;
		if (alreadyVisited->size() == 1) {
			ss << defaults->Stringify(overrides) << std::endl;
		}

		ss << std::endl << "#line 1 \"" << source << "\"" << std::endl; // Reset line numbers

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
				std::cout << source << ": Warning: #include detected without include file!" << std::endl;
			}
			else {
				size_t endIndx;
				size_t startIndx;

				startIndx = quotesIt - &contents[0] + 1;
				endIndx = contents.find('\"', startIndx);
				bGlobalSearch = false;
				
				if (endIndx == std::string::npos) {
					std::cout << source << ": Warning: unmatched quote in #include!" << std::endl;
					return;
				}

				last_include_pos = endIndx + 1;

				std::string includeSource = contents.substr(startIndx, endIndx - startIndx);
				std::string nextPath = path;

				if (bGlobalSearch) {
					nextPath = ".";

					size_t separator_i = includeSource.rfind('/');
					if (separator_i != std::string::npos) {
						nextPath = includeSource.substr(0, separator_i);
					}

					Load(includeSource, fileLoader, nextPath, defaults, overrides, streamOut,
						output, alreadyVisited);

				} else {
					size_t separator_i = includeSource.rfind('/');
					if (separator_i != std::string::npos) {
						nextPath = path + '/' + includeSource.substr(0, separator_i);
					}
					includeSource = path + '/' + includeSource;
				}

				Load(includeSource, fileLoader, nextPath, defaults, overrides, streamOut,
					output, alreadyVisited);
				
				ss << std::endl << "\n#line " << current_line + 1 << " \"" << source << "\"" << std::endl; // Reset line numbers
			}

			include_pos = contents.find("#include", include_pos + 1);
		}

		ss << contents.substr(last_include_pos);
	}

	void ShaderPreprocessor::Load(const std::string& source,
		IVirtualFileSystem* fileLoader,
		ShaderPreprocessorOutput* output,
		const ShaderPreprocessorConfig* defaults, 
		const ShaderPreprocessorConfig* overrides) {
		std::set<std::string> alreadyVisited;
		std::string path = ".";

		if (source.length() == 0)
			throw std::runtime_error("source has length 0!");

		size_t separator_i = source.rfind('/');
		if (separator_i != std::string::npos) {
			path = source.substr(0, separator_i);
		}

		std::stringstream streamOut;

		Load(source, fileLoader, path, 
			defaults, overrides,
			&streamOut, output, &alreadyVisited);

		output->mContent = streamOut.str();
	}
}