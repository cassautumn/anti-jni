#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <thread>
#include <iostream>
#include <vector>

#include "jni.h"
#include "jvmti.h"

#include "minhook/minhook.h"

namespace sdk
{
	JNIEnv* jni_env;
	void* original_1;

	auto wrapper_1(JNIEnv* arg1, jclass arg2, long arg3, void* arg4) -> void __stdcall
	{
		sdk::jni_env = arg1;

		((void(_stdcall*)(long))arg4)(arg3);
	}

	auto get_jni_object() -> JNIEnv*
	{
		auto pointer_1 = GetProcAddress(GetModuleHandleA("lwjgl64.dll"), "Java_org_lwjgl_opengl_GL11_nglMatrixMode");

		MH_CreateHook(pointer_1, sdk::wrapper_1, (void**)&sdk::original_1), MH_EnableHook(pointer_1);

		for (; sdk::jni_env == 0;)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

		MH_DisableHook(pointer_1);

		return sdk::jni_env;
	}

	auto write_fn_list(JNIEnv* object) -> void
	{
		auto get_fn_entry = *(unsigned __int64*)((unsigned __int64)object);

		auto first = get_fn_entry + 0x20;

		/* null out the list, will just make it crash */

		for (auto addr = first; *(unsigned __int64*)addr != 0; addr += 0x8)
			*(unsigned __int64*)addr = 0;
	}
}

auto init(void* base) -> __int32 __stdcall
{
	AllocConsole(), MH_Initialize();

	auto f1 = freopen("CONIN$", "r", stdin);
	auto f2 = freopen("CONOUT$", "w", stderr);
	auto f3 = freopen("CONOUT$", "w", stdout);
	
	auto object = sdk::get_jni_object(/* contains the inlined functions */);

	sdk::write_fn_list(object);

	for (; !GetAsyncKeyState(0x23);)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	MH_Uninitialize(), FreeLibraryAndExitThread((HINSTANCE)base, 0);

	return 0;
}

auto DllMain(void* base, unsigned int reason_to_call, void* reserved) -> bool __stdcall /* x64 release */
{
	if (reason_to_call == DLL_PROCESS_ATTACH)
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)init, base, 0, 0);

	return true;
}
