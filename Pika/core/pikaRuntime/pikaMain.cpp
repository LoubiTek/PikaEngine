#include <iostream>
#include <cstdio>
#include <filesystem>

#include <glad/glad.h>
#include <windowSystemm/window.h>


#include "logs/assert.h"
#include "dllLoader/dllLoader.h"
#include "pikaImgui/pikaImgui.h"

#include <memoryArena/memoryArena.h>
#include <runtimeContainer/runtimeContainer.h>

#include <logs/log.h>
#include <logs/logWindow.h>

#include <editor/editor.h>
#include <shortcutApi/shortcutApi.h>
#include <globalAllocator/globalAllocator.h>

#include <containerManager/containerManager.h>
#include <staticVector.h>

static bool shouldClose = false;

#if defined(PIKA_WINDOWS)
#include <Windows.h>

BOOL WINAPI customConsoleHandlerRoutine(
	_In_ DWORD dwCtrlType
)
{

	if (dwCtrlType == CTRL_CLOSE_EVENT)
	{
		shouldClose = true;
	
		return true;
	}

	return false;
}

#endif


int main()
{

#if defined(PIKA_WINDOWS)

#if PIKA_ENABLE_CONSOLE

	{
		AllocConsole();
		(void)freopen("conin$", "r", stdin);
		(void)freopen("conout$", "w", stdout);
		(void)freopen("conout$", "w", stderr);
		std::cout.sync_with_stdio();

		//HWND hwnd = GetConsoleWindow(); //dissable console x button
		//HMENU hmenu = GetSystemMenu(hwnd, FALSE);
		//EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);
		
		SetConsoleCtrlHandler(0, true); //dissable ctrl+c shortcut in console
		SetConsoleCtrlHandler(customConsoleHandlerRoutine, true); //custom exti function on clicking x button on console
	}

#endif

#endif
	

#pragma region init global variables stuff
	pika::initShortcutApi();
#pragma endregion

#pragma region log
	pika::LogManager logs;
	logs.init(pika::LogManager::DefaultLogFile);

#pragma endregion
	//todo (in the future) increment id if it wasn't possible to copy the file
#pragma region load dll
	std::filesystem::path currentPath = std::filesystem::current_path();
	pika::LoadedDll loadedDll;
	PIKA_PERMA_ASSERT(loadedDll.tryToloadDllUntillPossible(0, logs, std::chrono::seconds(5)),
		"Couldn't load dll");
#pragma endregion
	
#pragma region pika imgui id manager
	pika::ImGuiIdsManager imguiIdsManager;
#pragma endregion

#pragma region push notification manager
#if !(defined(PIKA_PRODUCTION) && PIKA_REMOVE_PUSH_NOTIFICATION_IN_PRODUCTION)
	pika::PushNotificationManager pushNotificationManager; 
	pushNotificationManager.init();
	logs.pushNotificationManager = &pushNotificationManager;
#endif
#pragma endregion


#pragma region init window opengl imgui and context
	PIKA_PERMA_ASSERT(glfwInit(), "Problem initializing glfw");
	//glfwSetErrorCallback(error_callback); todo
	pika::PikaWindow window = {};
	window.create();

	PIKA_PERMA_ASSERT(gladLoadGL(), "Problem initializing glad");


	pika::initImgui(window.context);

	window.context.glfwMakeContextCurrentPtr = glfwMakeContextCurrent;
#pragma endregion

#pragma region container manager

	pika::ContainerManager containerManager;

	containerManager.init();

#pragma endregion

#pragma region init dll reaml
	loadedDll.gameplayStart_(window.context);

	
#pragma endregion



#pragma region shortcuts
	pika::ShortcutManager shortcutManager;
#pragma endregion

#pragma region editor
#if !(defined(PIKA_PRODUCTION) && PIKA_REMOVE_EDITOR_IN_PRODUCATION)
	pika::Editor editor; //todo remove editor in production
	editor.init(shortcutManager);
#endif
#pragma endregion

	
	auto container = containerManager.createContainer
		(loadedDll.containerInfo[1], loadedDll, logs);

	while (!shouldClose)
	{
		if (window.shouldClose())
		{
			shouldClose = true;
			break;
		}


	#pragma region start imgui
		pika::imguiStartFrame(window.context);
	#pragma endregion

	#pragma region clear screen
		glClear(GL_COLOR_BUFFER_BIT);
	#pragma endregion

	#pragma region editor stuff
	#if !(defined(PIKA_PRODUCTION) && PIKA_REMOVE_EDITOR_IN_PRODUCATION)
		editor.update(window.input, shortcutManager, logs, 
			pushNotificationManager, loadedDll, containerManager);
	#endif
	#pragma endregion

	#pragma region push notification
	#if !(defined(PIKA_PRODUCTION) && PIKA_REMOVE_PUSH_NOTIFICATION_IN_PRODUCTION)
		static bool pushNoticicationOpen = true;
		pushNotificationManager.update(pushNoticicationOpen);
	#endif
	#pragma endregion

	#pragma region container manager

	#if !(defined(PIKA_PRODUCTION) && PIKA_REMOVE_EDITOR_IN_PRODUCATION)
		if (editor.shouldReloadDll)
		{
			editor.shouldReloadDll = false;
			containerManager.reloadDll(loadedDll, window, logs);
		}
	#endif
		
		containerManager.update(loadedDll, window, logs);

	#pragma endregion

	#pragma region end imgui frame
		pika::imguiEndFrame(window.context);
	#pragma endregion

	#pragma region window update
		window.update();
	#pragma endregion

	#pragma region shortcut manager update
		shortcutManager.update(window.input);
	#pragma endregion
	
	}

	containerManager.destroyAllContainers(loadedDll, logs);


	//terminate();

	return 0;
}