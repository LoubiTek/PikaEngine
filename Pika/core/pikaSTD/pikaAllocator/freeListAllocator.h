//////////////////////////////////////////////////
//freeListAllocator.h				beta 0.2
//Copyright(c) 2020 Luta Vlad
//https://github.com/meemknight/freeListAllocator
//////////////////////////////////////////////////

#pragma once

#include <pikaConfig.h>
#include <pikaSizes.h>

namespace pika
{

namespace memory
{


	#ifdef PIKA_WINDOWS
	
		struct FreeListAllocatorMutex
		{
		
			FreeListAllocatorMutex();
		
			void lock();
			void unlock();
		
			long counter;
			void* semaphore;
		
			~FreeListAllocatorMutex();
		
		};
	
	#elif defined(PIKA_LINUX)
	
	//todo add linux support
	
	#endif
	
	
	
	struct FreeListAllocator
	{
		char* baseMemory = 0;
	
		FreeListAllocator() = default;
		FreeListAllocator(void* baseMemory, size_t memorySize)
		{
			init(baseMemory, memorySize);
		}
	
		void init(void* baseMemory, size_t memorySize);
	
		void* allocate(size_t size);
	
		void free(void* mem);
	
		void* threadSafeAllocate(size_t size);
	
		void threadSafeFree(void* mem);
	
		//available memory is the free memory
		//biggest block is how large is the biggest free memory block
		//you can allocate less than the largest biggest free memory because 16 bytes are reserved per block
		void calculateMemoryMetrics(size_t& availableMemory, size_t& biggestBlock, int& freeBlocks);
	
	
		//if this is false it will crash if it is out of memory
		//if this is true it will return 0 when there is no more memory
		//I rocommand leaving this to false
		bool returnZeroIfNoMoreMemory = false;
	
	private:
	
		void* end = 0;
	
		FreeListAllocatorMutex mu;
	
		size_t getEnd()
		{
			return (size_t)end;
		}
	
	};


};

};