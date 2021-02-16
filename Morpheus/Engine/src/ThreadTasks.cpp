#include <Engine/ThreadTasks.hpp>

#include <fstream>

namespace Morpheus {
	TaskId ReadFileToMemoryJobDeferred(const std::string& filename, 
		TaskQueueInterface* queue, 
		PipeId* pipeOut, 
		TaskBarrier* barrier,
		bool binary) {
		
		auto pipe = queue->MakePipe();
		auto task = queue->MakeTask([pipe, filename, binary](const TaskParams& params) {

			char* buffer = nullptr;
			std::ifstream stream;

			try {
				if (binary)
					stream = std::ifstream(filename, std::ios::binary | std::ios::ate);
				else 
					stream = std::ifstream(filename);
					
				size_t data_size = 0;

				if (stream.is_open()) {
					std::streamsize size = stream.tellg();
					data_size = (size_t)(size + 1);
					stream.seekg(0, std::ios::beg);

					buffer = new char[data_size];
					if (!stream.read(buffer, size))
					{
						std::runtime_error("Could not read file");
					}

					stream.close();

					buffer[size] = 0;
				}
				else {
					throw std::runtime_error("Could not open file");
				}

				// Write data to pipe
				params.mPool->WritePipe(pipe, new ReadFileToMemoryResult((unsigned char*)buffer, data_size));
			} catch (...) {
				if (stream.is_open())
					stream.close();

				if (buffer)
					delete[] buffer;

				// Write exception to pipe
				params.mPool->WritePipeException(pipe, std::current_exception());
			}
		}, barrier, ASSIGN_THREAD_IO);

		queue->Dependencies(pipe).After(task);

		*pipeOut = pipe;

		return task;
	}
}