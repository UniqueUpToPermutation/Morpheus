#include <Engine/ThreadTasks.hpp>

#include <fstream>

namespace Morpheus {
	void ReadFileToMemoryJob(const std::string& filename, 
		TaskQueueInterface* queue, 
		TaskId* taskOut, 
		PipeId* pipeOut, 
		bool bWakeIOThread) {
		
		auto pipe = queue->MakePipe();
		auto task = queue->IOTask([pipe, filename](const TaskParams& params) {

			char* buffer = nullptr;
			std::ifstream stream;

			try {
				stream = std::ifstream(filename, std::ios::binary | std::ios::ate);

				if (stream.is_open()) {
					std::streamsize size = stream.tellg();
					stream.seekg(0, std::ios::beg);

					buffer = new char[size];
					if (!stream.read(buffer, size))
					{
						std::runtime_error("Could not read file");
					}

					stream.close();
				}
				else {
					throw std::runtime_error("Could not open file");
				}

				// Write data to pipe
				params.mPool->WritePipe(pipe, buffer);
			} catch (...) {
				if (stream.is_open())
					stream.close();

				if (buffer)
					delete[] buffer;

				// Write exception to pipe
				params.mPool->WritePipeException(pipe, std::current_exception());
			}
		}, bWakeIOThread);
	}
}