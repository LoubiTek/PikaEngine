#include "containerManager.h"
#include "containerManager.h"
#include "containerManager.h"
#include <globalAllocator/globalAllocator.h>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <fileManipulation/fileManipulation.h>
#include <stringManipulation/stringManipulation.h>
#include <filesystem>
#include <imgui.h>

pika::containerId_t pika::ContainerManager::createContainer(std::string containerName, 
	pika::LoadedDll &loadedDll, pika::LogManager &logManager, 
	pika::pikaImgui::ImGuiIdsManager &imguiIDsManager, size_t memoryPos)
{
	
	for(auto &i : loadedDll.containerInfo)
	{
		
		if (i.containerName == containerName)
		{
			return createContainer(i, loadedDll, logManager, imguiIDsManager, memoryPos);
		}

	}
	
	logManager.log(("Couldn't create container, couldn't find the name: " + containerName).c_str(), pika::logError);

	return 0;
}

bool pika::ContainerManager::setSnapshotToContainer(pika::containerId_t containerId, const char *snapshotName, pika::LogManager &logManager
	, pika::pikaImgui::ImGuiIdsManager &imguiIdManager)
{
	auto c = runningContainers.find(containerId);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for setting snapshot: #") 
			+ std::to_string(containerId)).c_str(),
			pika::logError);
		return false;
	}

	if (!checkIfSnapshotIsCompatible(c->second, snapshotName))
	{
		logManager.log((std::string("Snapshot incompatible: ") + snapshotName).c_str(),
			pika::logError);
		return false;
	}
	
	std::string file = PIKA_ENGINE_RESOURCES_PATH;
	file += snapshotName;
	file += ".snapshot";

	auto s = pika::getFileSize(file.c_str());

	if (s != (c->second.totalSize + sizeof(pika::RuntimeContainer)) ) 
	{
		logManager.log((std::string("Snapshot corrupted probably, file size incorrect: ") 
			+ snapshotName).c_str(),
			pika::logError);
		return false;
	}

	pika::readEntireFile(file.c_str(), c->second.getBaseAdress(),
		c->second.totalSize, sizeof(pika::RuntimeContainer));

	//c->second.requestedContainerInfo.requestedImguiIds 
	//	= imguiIdManager.getImguiIds(c->second.requestedContainerInfo.imguiTotalRequestedIds);

	logManager.log("Loaded snapshot");
	return true;
}

bool pika::ContainerManager::setRecordingToContainer(pika::containerId_t containerId, const char *recordingName, 
	pika::LogManager &logManager, pika::pikaImgui::ImGuiIdsManager &imguiIdManager)
{
	auto c = runningContainers.find(containerId);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for setting recording: #")
			+ std::to_string(containerId)).c_str(),
			pika::logError);
		return false;
	}
	//todo not reset imgui id on input recording
	if (!setSnapshotToContainer(containerId, recordingName, logManager, imguiIdManager))
	{
		return false;
	}

	c->second.flags.status = pika::RuntimeContainer::FLAGS::STATUS_BEING_PLAYBACK;
	c->second.flags.frameNumber = 0;
	pika::strlcpy(c->second.flags.recordingName, recordingName, sizeof(c->second.flags.recordingName));

}

//todo use regions further appart in production
void* pika::ContainerManager::allocateContainerMemory(pika::RuntimeContainer &container,
	pika::ContainerInformation containerInformation, void *memPos)
{
	size_t memoryRequired = containerInformation.calculateMemoryRequirements();

	void * baseMemory = allocateOSMemory(memoryRequired, memPos);

	if (baseMemory == nullptr) { return 0; }

	container.totalSize = memoryRequired;

	allocateContainerMemoryAtBuffer(container,
		containerInformation, baseMemory);

	return baseMemory;
}

void pika::ContainerManager::allocateContainerMemoryAtBuffer(pika::RuntimeContainer &container,
	pika::ContainerInformation containerInformation, void *buffer)
{
	const size_t staticMemory = containerInformation.containerStructBaseSize;
	const size_t heapMemory = containerInformation.containerStaticInfo.defaultHeapMemorySize;

	char *currentMemoryAdress = (char *)buffer;

	container.arena.containerStructMemory.size = staticMemory;
	container.arena.containerStructMemory.block = currentMemoryAdress;
	currentMemoryAdress += staticMemory;
	pika::align64(currentMemoryAdress);

	container.allocator.init(currentMemoryAdress, heapMemory);

	currentMemoryAdress += heapMemory;

	for (int i = 0; i < containerInformation.containerStaticInfo.bonusAllocators.size(); i++)
	{
		pika::align64(currentMemoryAdress);

		pika::memory::FreeListAllocator allocator;
		allocator.init(
			currentMemoryAdress,
			containerInformation.containerStaticInfo.bonusAllocators[i]
		);
		container.bonusAllocators.push_back(allocator);
		currentMemoryAdress += containerInformation.containerStaticInfo.bonusAllocators[i];
	}

}

void pika::ContainerManager::freeContainerStuff(pika::RuntimeContainer &container)
{
	deallocateOSMemory(container.arena.containerStructMemory.block);

	//container.arena.dealocateStaticMemory(); //static memory
	//deallocateOSMemory(container.allocator.originalBaseMemory); //heap memory
	//
	//for (auto &i : container.bonusAllocators)
	//{
	//	deallocateOSMemory(i.originalBaseMemory);
	//}
}

pika::containerId_t pika::ContainerManager::createContainer
(pika::ContainerInformation containerInformation,
	pika::LoadedDll &loadedDll, pika::LogManager &logManager, pika::pikaImgui::ImGuiIdsManager &imguiIDsManager,
	size_t memoryPos)
{	
	containerId_t id = ++idCounter;
	
	//not necessary if this is the only things that assigns ids.
	//if (runningContainers.find(id) != runningContainers.end())
	//{
	//	logManager.log((std::string("Container id already exists: #") + std::to_string(id)).c_str(), pika::logError);
	//	return false;
	//}

	//todo a create and destruct wrapper

	pika::RuntimeContainer container = {};
	pika::strlcpy(container.baseContainerName, containerInformation.containerName,
		sizeof(container.baseContainerName));
	
	if (!allocateContainerMemory(container, containerInformation, (void*)memoryPos))
	{
		logManager.log((std::string("Couldn't allocate memory for constructing container: #") 
			+ std::to_string(id)).c_str(), pika::logError);
		return 0;
	}

	if (containerInformation.containerStaticInfo.requestImguiFbo)
	{
		container.requestedContainerInfo.requestedFBO.createFramebuffer(400, 400); //todo resize small or sthing
		container.imguiWindowId = imguiIDsManager.getImguiIds();
	}


	loadedDll.bindAllocatorDllRealm(&container.allocator);
	
	//this calls the constructors (from the dll realm)
	if (!loadedDll.constructRuntimeContainer(container, containerInformation.containerName.c_str()))
	{
		loadedDll.resetAllocatorDllRealm();

		logManager.log((std::string("Couldn't construct container: #") + std::to_string(id)).c_str(), pika::logError);

		freeContainerStuff(container);

		container.requestedContainerInfo.requestedFBO.deleteFramebuffer();

		return 0;
	}


	loadedDll.resetAllocatorDllRealm();


#pragma region setup requested container info

	container.requestedContainerInfo.mainAllocator = &container.allocator;
	container.requestedContainerInfo.bonusAllocators = &container.bonusAllocators;
	container.requestedContainerInfo.requestedImguiIds =
		imguiIDsManager.getImguiIds(containerInformation.containerStaticInfo.requestImguiIds);
	container.requestedContainerInfo.imguiTotalRequestedIds = containerInformation.containerStaticInfo.requestImguiIds;

#pragma endregion

	
	loadedDll.bindAllocatorDllRealm(&container.allocator);
	container.pointer->create(container.requestedContainerInfo); //this calls create() (from the dll realm)
	loadedDll.resetAllocatorDllRealm();//sets the global allocator back to standard (used for runtime realm)

	runningContainers[id] = container;

	logManager.log(("Created container: " + std::string(container.baseContainerName) ).c_str());

	return id;
}

void pika::ContainerManager::init()
{
}

void pika::ContainerManager::update(pika::LoadedDll &loadedDll, pika::PikaWindow &window,
	pika::LogManager &logs, pika::pikaImgui::ImGuiIdsManager &imguiIdManager, std::streambuf *consoleBuffer)
{
	PIKA_DEVELOPMENT_ONLY_ASSERT(loadedDll.dllHand != 0, "dll not loaded when trying to update containers");

#pragma region reload dll


	//todo try to recover from a failed load

	if (loadedDll.shouldReloadDll())
	{
		reloadDll(loadedDll, window, logs, consoleBuffer); //todo return 0 on fail
		consoleBuffer = loadedDll.getConsoleBuffer_();

		//todo mark shouldCallReaload or just call reload
		
	}

	
	
#pragma endregion


#pragma region running containers
	for (auto &c : runningContainers)
	{

		if (c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_RUNNING
			||
			c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_BEING_RECORDED
			||
			c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_BEING_PLAYBACK
			)
		{
			
			PIKA_DEVELOPMENT_ONLY_ASSERT(
				(c.second.requestedContainerInfo.requestedFBO.fbo == 0 &&
				c.second.imguiWindowId == 0) ||
				(
				c.second.requestedContainerInfo.requestedFBO.fbo != 0 &&
				c.second.imguiWindowId != 0), "we have a fbo but no imguiwindow id"
			);

			auto windowInput = window.input;

		#if PIKA_DEVELOPMENT

			if (c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_BEING_RECORDED)
			{
				if (!makeRecordingStep(c.first, logs, windowInput))
				{

					c.second.flags.status = pika::RuntimeContainer::FLAGS::STATUS_RUNNING;

					logs.log((std::string("Stopped container recording because we couldn't oppen file") 
						+ std::to_string(c.first)).c_str(),
						pika::logError);
				}
			}
			
			
			
		#endif

			if (c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_BEING_PLAYBACK)
			{
				pika::Input readInput;

				std::string fileName = c.second.flags.recordingName;
				fileName += ".recording";
				fileName = PIKA_ENGINE_RESOURCES_PATH + fileName;

				auto s = pika::getFileSize(fileName.c_str());
				if (c.second.flags.frameNumber * sizeof(pika::Input) >= s && s != 0)
				{
					//todo optional logs here
					if (!setSnapshotToContainer(c.first, c.second.flags.recordingName, logs, imguiIdManager))
					{
						logs.log((std::string("Stopped container playback because we couldn't assign it's snapshot on frame 0")
							+ std::to_string(c.first)).c_str(),
							pika::logError);
						c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_RUNNING;
						goto endContainerErrorChecking;
					}
					c.second.flags.frameNumber = 0;
				}

				if (s == 0)
				{
					logs.log((std::string("Stopped container playback because we couldn't oppen file or its content is empty")
						+ std::to_string(c.first)).c_str(),
						pika::logError);
					c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_RUNNING;
				}
				else if (s % sizeof(pika::Input) != 0)
				{
					logs.log((std::string("Stopped container playback because the file content is corrupt")
						+ std::to_string(c.first)).c_str(),
						pika::logError);
					c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_RUNNING;
				}
				if (!pika::readEntireFile(fileName.c_str(), &readInput, sizeof(pika::Input), sizeof(pika::Input) * c.second.flags.frameNumber))
				{
					logs.log((std::string("Stopped container playback because we couldn't oppen file")
						+ std::to_string(c.first)).c_str(),
						pika::logError);
					c.second.flags.status == pika::RuntimeContainer::FLAGS::STATUS_RUNNING;
				}
				else
				{
					windowInput = readInput;
					c.second.flags.frameNumber++;
				}


			}

			endContainerErrorChecking:


		#if PIKA_PRODUCTION
			constexpr bool isProduction = 1;
		#else
			constexpr bool isProduction = 0;
		#endif

			if (c.second.imguiWindowId && !isProduction)
			{

				ImGui::PushID(c.second.imguiWindowId);
				
				ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 1.0f));
				ImGui::SetNextWindowSize({200,200}, ImGuiCond_Once);
				ImGui::Begin( (std::string("gameplay window id: ") + std::to_string(c.first)).c_str(),
					0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				
				auto s = ImGui::GetContentRegionMax();

				ImGui::Image((void *)c.second.requestedContainerInfo.requestedFBO.texture, s, {0, 1}, {1, 0},
					{1,1,1,1}, {0,0,0,1});

				ImGui::End();

				ImGui::PopStyleColor();

				ImGui::PopID();

				auto windowState = window.windowState;
				windowState.w = s.x;
				windowState.h = s.y;

				c.second.requestedContainerInfo.requestedFBO.resizeFramebuffer(windowState.w, windowState.h);

				glBindFramebuffer(GL_FRAMEBUFFER, c.second.requestedContainerInfo.requestedFBO.fbo);

				loadedDll.bindAllocatorDllRealm(&c.second.allocator);
				c.second.pointer->update(windowInput, windowState, c.second.requestedContainerInfo);
				loadedDll.resetAllocatorDllRealm();

				glBindFramebuffer(GL_FRAMEBUFFER, 0);

			}
			else
			{
				loadedDll.bindAllocatorDllRealm(&c.second.allocator);
				c.second.pointer->update(windowInput, window.windowState, c.second.requestedContainerInfo);
				loadedDll.resetAllocatorDllRealm();
			}


		}
		else
		{
			//still keep it running on the same frame mabe
		}

	}
#pragma endregion

}

void pika::ContainerManager::reloadDll(pika::LoadedDll &loadedDll, pika::PikaWindow &window, pika::LogManager &logs,
	std::streambuf *consoleBuffer)
{

	std::this_thread::sleep_for(std::chrono::milliseconds(200)); // make sure that the compiler had enough time 
		//to get hold onto the dll 


	//pika::LoadedDll newDll;
	//if (!newDll.tryToloadDllUntillPossible(loadedDll.id + 1, logs, std::chrono::seconds(1)))
	//{
	//	logs.log("Dll reload attemp failed", pika::logWarning);
	//	newDll.unloadDll();
	//	return;
	//}
	//else
	//{
	//	newDll.unloadDll();
	//}
	//std::this_thread::sleep_for(std::chrono::milliseconds(10)); // make sure that the dll is unloaded


	auto oldContainerInfo = loadedDll.containerInfo;

	if (!loadedDll.tryToloadDllUntillPossible(loadedDll.id, logs, std::chrono::seconds(5)))
	{
		logs.log("Couldn't reloaded dll", pika::logWarning);
		return;
	}
	//todo pospone dll reloading

	std::unordered_map<std::string, pika::ContainerInformation> containerNames;
	for (auto &c : loadedDll.containerInfo)
	{
		containerNames[c.containerName] = c;
	}

	std::unordered_map<std::string, pika::ContainerInformation> oldContainerNames;
	for (auto &c : oldContainerInfo)
	{
		oldContainerNames[c.containerName] = c;
	}

	//clear containers that dissapeared
	{


		std::vector<pika::containerId_t> containersToClean;
		for (auto &i : runningContainers)
		{
			if (containerNames.find(i.second.baseContainerName) ==
				containerNames.end())
			{
				std::string l = "Killed container because it does not exist anymore in dll: " + 
					std::string(i.second.baseContainerName)
					+ " #" + std::to_string(i.first);
				logs.log(l.c_str(), pika::logError);

				containersToClean.push_back(i.first);
			}
		}

		for (auto i : containersToClean)
		{
			forceTerminateContainer(i, loadedDll, logs);
		}
	}

	//clear containers that changed static info
	{

		std::vector<pika::containerId_t> containersToClean;
		for (auto &i : runningContainers)
		{

			auto &newContainer = containerNames[i.second.baseContainerName];
			auto &oldContainer = oldContainerNames[i.second.baseContainerName];

			if (newContainer != oldContainer)
			{
				std::string l = "Killed container because its static container info\nhas changed: "
					+ std::string(i.second.baseContainerName)
					+ " #" + std::to_string(i.first);
				logs.log(l.c_str(), pika::logError);

				containersToClean.push_back(i.first);
			}

		}

		for (auto i : containersToClean)
		{
			forceTerminateContainer(i, loadedDll, logs);
		}

	}


	loadedDll.gameplayReload_(window.context);

	logs.log("Reloaded dll");

}

bool pika::ContainerManager::destroyContainer(containerId_t id, pika::LoadedDll &loadedDll,
	pika::LogManager &logManager)
{
	PIKA_DEVELOPMENT_ONLY_ASSERT(loadedDll.dllHand != 0, "dll not loaded when trying to destroy container");

	auto c = runningContainers.find(id);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for destruction: #") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}

	auto name = c->second.baseContainerName;

	loadedDll.bindAllocatorDllRealm(&c->second.allocator);
	loadedDll.destructContainer_(&(c->second.pointer), &c->second.arena);
	loadedDll.resetAllocatorDllRealm();

	freeContainerStuff(c->second);

	c->second.requestedContainerInfo.requestedFBO.deleteFramebuffer();

	runningContainers.erase(c);

	logManager.log((std::string("Destroyed continer: ") + name + " #" + std::to_string(id)).c_str());

	return true;
}

//todo remove some of this functions in production


//snapshot file format:
//
// binary
// 
// pika::RuntimeContainer 
// 
// static memory
// 
// heap memory
// 
//
bool pika::ContainerManager::makeSnapshot(containerId_t id, pika::LogManager &logManager, const char *fileName)
{
	auto c = runningContainers.find(id);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for making snapshot: #") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}


	std::string filePath = PIKA_ENGINE_RESOURCES_PATH;
	filePath += fileName;

	filePath += ".snapshot";
		

	if(!pika::writeEntireFile(filePath.c_str(), &c->second, sizeof(c->second)))
	{
		logManager.log(("Couldn't write to file for making snapshot: " + filePath).c_str(),
			pika::logError);
		return false;
	}

	if (!pika::appendToFile(filePath.c_str(), 
		c->second.getBaseAdress(), c->second.totalSize))
	{
		pika::deleteFile(filePath.c_str());
		logManager.log(("Couldn't write to file for making snapshot: " + filePath).c_str(),
			pika::logError);
		return false;
	}


	//if (!pika::appendToFile(filePath.c_str(), 
	//	c->second.arena.containerStructMemory.block, c->second.arena.containerStructMemory.size))
	//{
	//	pika::deleteFile(filePath.c_str());
	//	logManager.log(("Couldn't write to file for making snapshot: " + filePath).c_str(),
	//		pika::logError);
	//	return false;
	//}
	//
	//if (!pika::appendToFile(filePath.c_str(),
	//	c->second.allocator.originalBaseMemory, c->second.allocatorSize))
	//{
	//	pika::deleteFile(filePath.c_str());
	//	logManager.log(("Couldn't write to file for making snapshot: " + filePath).c_str(),
	//		pika::logError);
	//	return false;
	//}


	return true;
}

bool pika::ContainerManager::startRecordingContainer(containerId_t id, pika::LogManager &logManager, const char *fileName)
{
	auto c = runningContainers.find(id);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for making recording: #") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}

	if(c->second.flags.status != pika::RuntimeContainer::FLAGS::STATUS_RUNNING)
	{
		logManager.log((std::string("Trying to record a container that is not running (on status): #") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}


	std::string filePath = PIKA_ENGINE_RESOURCES_PATH;
	filePath += fileName;

	filePath += ".recording";

	if (filePath.size() > sizeof(c->second.flags.recordingName) - 1)
	{
		logManager.log((std::string("File path too big (on trying to record)") + std::to_string(id)).c_str(),
			pika::logError);
		return 0;
	}

	if (!makeSnapshot(id, logManager, fileName)) 
	{
		logManager.log((std::string("Couldn't make snapshot for starting recording") + std::to_string(id)).c_str(),
			pika::logError);
		return 0;
	}



	c->second.flags.status = pika::RuntimeContainer::FLAGS::STATUS_BEING_RECORDED;
	pika::strlcpy(c->second.flags.recordingName, filePath, sizeof(c->second.flags.recordingName));



}

bool pika::ContainerManager::stopRecordingContainer(containerId_t id, pika::LogManager &logManager)
{
	auto c = runningContainers.find(id);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for stopping recording: #") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}

	c->second.flags.status = pika::RuntimeContainer::FLAGS::STATUS_RUNNING;

}

bool pika::ContainerManager::makeRecordingStep(containerId_t id, pika::LogManager &logManager,
	pika::Input &input)
{
	auto c = runningContainers.find(id);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for making recording step: #") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}

	if (!pika::appendToFile(c->second.flags.recordingName, &input, sizeof(input)))
	{
		logManager.log((std::string("Couldn't append to file for recording container") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}

	return true;
}

bool pika::ContainerManager::forceTerminateContainer(containerId_t id, pika::LoadedDll &loadedDll, pika::LogManager &logManager)
{
	PIKA_DEVELOPMENT_ONLY_ASSERT(loadedDll.dllHand != 0, "dll not loaded when trying to destroy container");

	auto c = runningContainers.find(id);
	if (c == runningContainers.end())
	{
		logManager.log((std::string("Couldn't find container for destruction: #") + std::to_string(id)).c_str(),
			pika::logError);
		return false;
	}

	auto name = c->second.baseContainerName;

	freeContainerStuff(c->second);

	runningContainers.erase(c);

	logManager.log((std::string("Force terminated continer: ") + name + " #" + std::to_string(id)).c_str());

	c->second.requestedContainerInfo.requestedFBO.deleteFramebuffer();

	return true;
}

void pika::ContainerManager::destroyAllContainers(pika::LoadedDll &loadedDll,
	pika::LogManager &logManager)
{
	std::vector < pika::containerId_t> containersId;
	containersId.reserve(runningContainers.size());

	for (auto &c : runningContainers)
	{
		containersId.push_back(c.first);
	}

	for (auto i : containersId)
	{
		destroyContainer(i, loadedDll, logManager);
	}

}

#ifdef PIKA_PRODUCTION

void *pika::ContainerManager::allocateOSMemory(size_t size, void *baseAdress)
{
	PIKA_PERMA_ASSERT(baseAdress == nullptr, "can't allocate fixed memory in production");
	return malloc(size);
}

void pika::ContainerManager::deallocateOSMemory(void *baseAdress)
{
	free(baseAdress);
}

#else

#include <Windows.h>

void *pika::ContainerManager::allocateOSMemory(size_t size, void *baseAdress)
{
	return VirtualAlloc(baseAdress, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void pika::ContainerManager::deallocateOSMemory(void *baseAdress)
{
	VirtualFree(baseAdress, 0, MEM_RELEASE);
}



#endif

std::vector<std::string> pika::getAvailableSnapshots(pika::RuntimeContainer &info)
{
	std::vector<std::string> files;

	auto curDir = std::filesystem::directory_iterator(PIKA_ENGINE_RESOURCES_PATH);

	for (const auto &iter : curDir)
	{
		if (std::filesystem::is_regular_file(iter)
			&& iter.path().extension() == ".snapshot"
			)
		{
			if (pika::checkIfSnapshotIsCompatible(info, iter.path().stem().string().c_str()))
			{
				files.push_back(iter.path().stem().string());
			}

		}
	}

	return files;
}

std::vector<std::string> pika::getAvailableRecordings(pika::RuntimeContainer &info)
{
	auto snapshots = getAvailableSnapshots(info);
	
	std::vector<std::string> returnVec;
	returnVec.reserve(snapshots.size());

	auto curDir = std::filesystem::directory_iterator(PIKA_ENGINE_RESOURCES_PATH);

	for (const auto &iter : curDir)
	{
		if (std::filesystem::is_regular_file(iter)
			&& iter.path().extension() == ".recording"
			)
		{

			if (std::find(snapshots.begin(), snapshots.end(), iter.path().stem().string()) != snapshots.end())
			{
				returnVec.push_back(iter.path().stem().string());
			}
		}
	}

	return returnVec;
}

std::vector<std::string> pika::getAvailableSnapshotsAnyMemoryPosition(pika::RuntimeContainer &info)
{
	std::vector<std::string> files;

	auto curDir = std::filesystem::directory_iterator(PIKA_ENGINE_RESOURCES_PATH);

	for (const auto &iter : curDir)
	{
		if (std::filesystem::is_regular_file(iter)
			&& iter.path().extension() == ".snapshot"
			)
		{
			if (pika::checkIfSnapshotIsCompatibleAnyMemoryPosition(info, iter.path().stem().string().c_str()))
			{
				files.push_back(iter.path().stem().string());
			}

		}
	}

	return files;
}

std::vector<std::string> pika::getAvailableRecordingAnyMemoryPosition(pika::RuntimeContainer &info)
{
	auto snapshots = getAvailableSnapshotsAnyMemoryPosition(info);

	std::vector<std::string> returnVec;
	returnVec.reserve(snapshots.size());

	auto curDir = std::filesystem::directory_iterator(PIKA_ENGINE_RESOURCES_PATH);

	for (const auto &iter : curDir)
	{
		if (std::filesystem::is_regular_file(iter)
			&& iter.path().extension() == ".recording"
			)
		{

			if (std::find(snapshots.begin(), snapshots.end(), iter.path().stem().string()) != snapshots.end())
			{
				returnVec.push_back(iter.path().stem().string());
			}
		}
	}

	return returnVec;
}

std::vector<std::string> pika::getAvailableSnapshotsAnyMemoryPosition(pika::ContainerInformation &info)
{
	std::vector<std::string> files;

	auto curDir = std::filesystem::directory_iterator(PIKA_ENGINE_RESOURCES_PATH);

	for (const auto &iter : curDir)
	{
		if (std::filesystem::is_regular_file(iter)
			&& iter.path().extension() == ".snapshot"
			)
		{
			if (pika::checkIfSnapshotIsCompatibleAnyMemoryPosition(info, iter.path().stem().string().c_str()))
			{
				files.push_back(iter.path().stem().string());
			}

		}
	}

	return files;
}

std::vector<std::string> pika::getAvailableRecordingsAnyMemoryPosition(pika::ContainerInformation &info)
{
	auto snapshots = getAvailableSnapshotsAnyMemoryPosition(info);

	std::vector<std::string> returnVec;
	returnVec.reserve(snapshots.size());

	auto curDir = std::filesystem::directory_iterator(PIKA_ENGINE_RESOURCES_PATH);

	for (const auto &iter : curDir)
	{
		if (std::filesystem::is_regular_file(iter)
			&& iter.path().extension() == ".recording"
			)
		{

			if (std::find(snapshots.begin(), snapshots.end(), iter.path().stem().string()) != snapshots.end())
			{
				returnVec.push_back(iter.path().stem().string());
			}
		}
	}

	return returnVec;
}

bool pika::checkIfSnapshotIsCompatible(pika::RuntimeContainer &info, const char *snapshotName)
{

	std::string file = PIKA_ENGINE_RESOURCES_PATH;
	file += snapshotName;
	file += ".snapshot";
	
	pika::RuntimeContainer loadedInfo = {};

	auto s = pika::readEntireFile(file.c_str(), &loadedInfo, sizeof(loadedInfo));

	if (s != sizeof(loadedInfo))
	{
		return 0;
	}

	//todo mabe a method here?
	if (loadedInfo.allocator.originalBaseMemory != info.allocator.originalBaseMemory)
	{
		return false;
	}

	if (loadedInfo.requestedContainerInfo.imguiTotalRequestedIds != info.requestedContainerInfo.imguiTotalRequestedIds)
	{
		return false;
	}

	if (loadedInfo.arena.containerStructMemory.block != info.arena.containerStructMemory.block)
	{
		return false;
	}

	if (loadedInfo.arena.containerStructMemory.size != info.arena.containerStructMemory.size)
	{
		return false;
	}

	if (std::strcmp(loadedInfo.baseContainerName, info.baseContainerName) != 0)
	{
		return false;
	}

	if (loadedInfo.requestedContainerInfo.bonusAllocators != info.requestedContainerInfo.bonusAllocators)
	{
		return false;
	}

	//check if user requested an imgui window
	if (
		!(
			(info.imguiWindowId == 0&&
			loadedInfo.imguiWindowId == 0
			)||
			(
			info.imguiWindowId != 0 &&
			loadedInfo.imguiWindowId != 0
			)
		)
		)
	{
		return false;
	}

	if (loadedInfo.totalSize != info.totalSize)
	{
		return false;
	}
		
	return true;
}

bool pika::checkIfSnapshotIsCompatibleAnyMemoryPosition(pika::RuntimeContainer &info, const char *snapshotName)
{
	std::string file = PIKA_ENGINE_RESOURCES_PATH;
	file += snapshotName;
	file += ".snapshot";

	pika::RuntimeContainer loadedInfo = {};

	auto s = pika::readEntireFile(file.c_str(), &loadedInfo, sizeof(loadedInfo));

	if (s != sizeof(loadedInfo))
	{
		return 0;
	}

	if (loadedInfo.arena.containerStructMemory.size != info.arena.containerStructMemory.size)
	{
		return false;
	}

	if (std::strcmp(loadedInfo.baseContainerName, info.baseContainerName) != 0)
	{
		return false;
	}

	if (loadedInfo.requestedContainerInfo.bonusAllocators != info.requestedContainerInfo.bonusAllocators)
	{
		return false;
	}

	//check if user requested an imgui window
	if (
		!(
		(info.imguiWindowId == 0 &&
		loadedInfo.imguiWindowId == 0
		) ||
		(
		info.imguiWindowId != 0 &&
		loadedInfo.imguiWindowId != 0
		)
		)
		)
	{
		return false;
	}

	if (loadedInfo.totalSize != info.totalSize)
	{
		return false;
	}

	return true;
}

bool pika::checkIfSnapshotIsCompatibleAnyMemoryPosition(pika::ContainerInformation &info, const char *snapshotName)
{
	std::string file = PIKA_ENGINE_RESOURCES_PATH;
	file += snapshotName;
	file += ".snapshot";

	pika::RuntimeContainer loadedInfo = {};

	auto s = pika::readEntireFile(file.c_str(), &loadedInfo, sizeof(loadedInfo));

	if (s != sizeof(loadedInfo))
	{
		return 0;
	}

	if (loadedInfo.arena.containerStructMemory.size != info.containerStructBaseSize)
	{
		return false;
	}

	if (std::strcmp(loadedInfo.baseContainerName, info.containerName.c_str()) != 0)
	{
		return false;
	}

	if (loadedInfo.bonusAllocators.size() != info.containerStaticInfo.bonusAllocators.size())
	{
		return false;
	}

	for (int i = 0; i < loadedInfo.bonusAllocators.size(); i++)
	{
		if (loadedInfo.bonusAllocators[i].heapSize != info.containerStaticInfo.bonusAllocators[i]) //todo this doesn't seem to work
		{
			return false;
		}
	}

	//check if user requested an imgui window
	if (
		!(
		(info.containerStaticInfo.requestImguiFbo == false &&
		loadedInfo.imguiWindowId == 0
		) ||
		(
		info.containerStaticInfo.requestImguiFbo == true &&
		loadedInfo.imguiWindowId != 0
		)
		)
		)
	{
		return false;
	}

	if (loadedInfo.allocator.heapSize != info.containerStaticInfo.defaultHeapMemorySize)
	{
		return false;
	}

	return true;
}

void *pika::getSnapshotMemoryPosition(const char *snapshotName)
{
	std::string file = PIKA_ENGINE_RESOURCES_PATH;
	file += snapshotName;
	file += ".snapshot";

	pika::RuntimeContainer loadedInfo = {};

	auto s = pika::readEntireFile(file.c_str(), &loadedInfo, sizeof(loadedInfo));

	if (s != sizeof(loadedInfo))
	{
		return nullptr;
	}

	return loadedInfo.getBaseAdress();
}
