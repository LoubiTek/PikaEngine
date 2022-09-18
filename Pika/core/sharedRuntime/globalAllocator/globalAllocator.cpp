#include "globalAllocator.h"
#include <malloc.h>
#include <pikaAllocator/freeListAllocator.h>


void *DefaultAllocator(size_t size)
{
	return malloc(size);
}
void DefaultFree(void *ptr)
{
	free(ptr);
}

pika::memory::FreeListAllocator *currentCustomAllocator = {};
void *CustomAllocator(size_t size)
{
	return currentCustomAllocator->allocate(size);
}
void CustomFree(void *ptr)
{
	currentCustomAllocator->free(ptr);
}

void* (*GlobalAllocateFunction)(size_t) = DefaultAllocator;
void  (*GlobalFree)(void *) = DefaultFree;

namespace pika
{
namespace memory
{
	void setGlobalAllocatorToStandard()
	{
		GlobalAllocateFunction = DefaultAllocator;
		GlobalFree = DefaultFree;
	}

	void setGlobalAllocator(pika::memory::FreeListAllocator *allocator)
	{
		currentCustomAllocator = allocator;
		GlobalAllocateFunction = CustomAllocator;
		GlobalFree = CustomFree;
	}
}
}



void *operator new  (size_t count)
{
	return GlobalAllocateFunction(count);
}

void *operator new[](size_t count)
{
	return GlobalAllocateFunction(count);
}

void operator delete  (void *ptr)
{

	GlobalFree(ptr);
}

void operator delete[](void *ptr)
{
	GlobalFree(ptr);
}