#include "editShortcuts.h"
#include <imgui.h>
#include <pikaImgui/pikaImgui.h>

void pika::EditShortcutsWindow::init()
{
}

void pika::EditShortcutsWindow::update(pika::ShortcutManager &shortcutManager, bool &open)
{


	ImGui::SetNextWindowSize({400, 500});

	ImGui::PushID(pika::pikaImgui::EditorImguiIds::editShortcutWindow);

	if (ImGui::Begin(ICON_NAME, &open,
		ImGuiWindowFlags_NoDocking | 
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse
		))
	{
		
		ImGui::Text("Edit shortcuts\n");

		//todo unique id
		if (ImGui::BeginChild(12, {}, true))
		{


			ImGui::Columns(2, 0, false);

			for (auto &shortcut : shortcutManager.registeredShortcuts)
			{

				ImGui::Text(shortcut.first.c_str());

				ImGui::NextColumn();


				char input[256] = {};
				std::strncpy(input, shortcut.second.shortcut.c_str(), sizeof(input));
				
				int flags = ImGuiInputTextFlags_EnterReturnsTrue;
				if (!shortcut.second.editable)
				{
					flags = flags | ImGuiInputTextFlags_ReadOnly;
				}

				if (
					ImGui::InputText(("##" + shortcut.first).c_str(),
					input, sizeof(input), flags)
					)
				{
					shortcut.second.shortcut = pika::normalizeShortcutName(input);

				}

				ImGui::NextColumn();
			}

			ImGui::Columns(1);

			ImGui::EndChild();
		}



	}
	ImGui::End();

	ImGui::PopID();

}

