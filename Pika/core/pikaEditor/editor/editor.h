#pragma once
//////////////////////////////////////////
//editor.h
//Luta Vlad(c) 2022
//https://github.com/meemknight/PikaEngine
//////////////////////////////////////////

#include <pikaImgui/pikaImgui.h>
#include <logs/logWindow.h>
#include <windowSystemm/input.h>
#include <shortcutApi/shortcutApi.h>
#include <editShortcuts/editShortcuts.h>
#include <pushNotification/pushNotification.h>
#include <containersWindow/containersWindow.h>

namespace pika
{

	struct Editor
	{

		void init(pika::ShortcutManager &shortcutManager);

		void update(const pika::Input &input, pika::ShortcutManager &shortcutManager
			,pika::LogManager &logs, pika::PushNotificationManager &pushNotificationManager,
			pika::LoadedDll &loadedDll, pika::ContainerManager &containerManager);

		struct
		{
			bool hideMainWindow = 0;
		}optionsFlags;

		struct
		{
			bool logsWindow = 0;
			bool editShortcutsWindow = 0;
			bool containerManager = 0;
		}windowFlags;

		pika::LogWindow logWindow;
		pika::EditShortcutsWindow editShortcutsWindow;
		pika::ContainersWindow containersWindow;

		bool lastHideWindowState = optionsFlags.hideMainWindow;

		bool shouldReloadDll = 0;
	};



}