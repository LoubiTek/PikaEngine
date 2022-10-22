#pragma once
#include <windowSystemm/window.h>
#include <iostream>
#include <pikaOptional.h>
#include <string>
#include <pikaAllocator/freeListAllocator.h>
#include <staticVector.h>
#include <pikaGL/frameBuffer.h>

#define READENTIREFILE(x) bool x(const char* name, void* buffer, size_t size)
typedef READENTIREFILE(readEntireFile_t);
#undef READENTIREFILE

#define GETFILESIZE(x) bool x(const char* name, size_t &size)
typedef GETFILESIZE(getFileSize_t);
#undef GETFILESIZE

static constexpr size_t MaxAllocatorsCount = 128;


//this is passed by the engine. You should not modify the data
//this is also used by the engine to give you acces to some io functions
struct RequestedContainerInfo
{
	pika::memory::FreeListAllocator *mainAllocator = {};
	pika::StaticVector<pika::memory::FreeListAllocator, MaxAllocatorsCount> *bonusAllocators = {};

	//readEntireFile_t *readEntireFilePointer = {};
	//getFileSize_t *getFileSizePointer = {};

	pika::GL::PikaFramebuffer requestedFBO = {};

	int requestedImguiIds = 0;
	int imguiTotalRequestedIds = 0;

	//todo implement
	bool readEntireFile(const char *name, void *buffer, size_t size)
	{
		//PIKA_DEVELOPMENT_ONLY_ASSERT(readEntireFilePointer, "read entire file pointer not assigned");

		//bool rez = readEntireFilePointer(name, buffer, size);

		//pika::memory::setGlobalAllocator(mainAllocator);

		//return rez;
	}

	bool getFileSize(const char *name, size_t &size)
	{
		//PIKA_DEVELOPMENT_ONLY_ASSERT(getFileSizePointer, "get file size pointer not assigned");



	}
};


struct ContainerStaticInfo
{

	//this is the main heap allocator memory size
	size_t defaultHeapMemorySize = 0;
	
	pika::StaticVector<size_t, MaxAllocatorsCount> bonusAllocators = {};

	//the engine will create a new window for your container and give you the fbo to bind to
	//in release that fbo will just be the default framebuffer
	bool requestImguiFbo = 0;

	unsigned int requestImguiIds = 0;

	bool _internalNotImplemented = 0;

	bool operator==(const ContainerStaticInfo &other)
	{
		if (this == &other) { return true; }

		return
			this->defaultHeapMemorySize == other.defaultHeapMemorySize &&
			this->bonusAllocators == other.bonusAllocators &&
			this->_internalNotImplemented == other._internalNotImplemented &&
			this->requestImguiFbo == other.requestImguiFbo;
			this->requestImguiIds == other.requestImguiIds;
		;
	}

	bool operator!=(const ContainerStaticInfo &other)
	{
		return !(*this == other);
	}


};


struct Container
{

	//this is used to give to the engine basic information about your container.
	//this function should be pure
	//this function should not allocate memory
	//this should not be dependent on anything that is called on create or library initialization
	static ContainerStaticInfo containerInfo() { ContainerStaticInfo c; c._internalNotImplemented = true; return c; };
	

	virtual void create(RequestedContainerInfo &requestedInfo) = 0;

	virtual void update(
		pika::Input input, 
		pika::WindowState windowState,
		RequestedContainerInfo &requestedInfo) = 0;

	virtual ~Container() {};

};