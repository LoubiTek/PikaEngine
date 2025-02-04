#pragma once
#include <pikaConfig.h>


#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"
#include <IconsForkAwesome.h>
#include <pikaAllocator/freeListAllocator.h>

#include <pikaContext.h>

#ifdef PIKA_WINDOWS
#define IM_PRId64   "I64d"
#define IM_PRIu64   "I64u"
#define IM_PRIx64   "I64X"
#else
#define IM_PRId64   "lld"
#define IM_PRIu64   "llu"
#define IM_PRIx64   "llX"
#endif

namespace pika
{
	namespace pikaImgui
	{

		void *imguiCustomAlloc(size_t sz, void *user_data);
		void imguiCustomFree(void *ptr, void *user_data);

		void setImguiAllocator(pika::memory::FreeListAllocator &allocator);

		void initImgui(PikaContext &pikaContext);
		void setImguiContext(PikaContext pikaContext);
		void imguiStartFrame(PikaContext pikaContext);
		void imguiEndFrame(PikaContext pikaContext);

		namespace EditorImguiIds
		{
			enum
			{

				idsCount = 4000
			};
		}

		struct ImGuiIdsManager
		{
			int counter = EditorImguiIds::idsCount + 1;

			//returns the first id. (count) ids will be reserved.
			//if you want 5 ids and the function returns 10, then ids 10 11 12 13 14 will be reserved.
			int getImguiIds(unsigned int count = 1)
			{
				if (count == 0) { return 0; }
				auto c = counter;
				counter += count;
				return c;
			}
		};

		void addErrorSymbol();
		void addWarningSymbol();

		void helpMarker(const char *desc);
		//todo another namespace for pika imgui adons

		void alignForWidth(float width, float alignment = 0.5f);
		
		void displayMemorySizeValue(size_t value);

		//todo move to internal
		void displayMemorySizeToggle();

	};
};
