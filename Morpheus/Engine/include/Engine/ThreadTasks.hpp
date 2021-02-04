#pragma once

#include <Engine/ThreadPool.hpp>


namespace Morpheus {
	void ReadFileToMemoryJob(const std::string& filename, 
		TaskQueueInterface* queue, 
		TaskId* taskOut, 
		PipeId* pipeOut,
		bool bWakeIOThread = true);
}