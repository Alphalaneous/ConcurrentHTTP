#include "includes.h"
#include <thread>
#include "base/Director.h"
#include "HttpClientH.h"

void(__thiscall* CCScheduler_update)(CCScheduler* self, float);

void __fastcall CCScheduler_update_H(CCScheduler* self, void*, float dt) {

    CCScheduler_update(self, dt);
    Director::getInstance()->getScheduler()->update(dt);
    
}


DWORD WINAPI thread_func(void* hModule) {
    MH_Initialize();

    CreateHook(CCAddr("?update@CCScheduler@cocos2d@@UAEXM@Z"), CCScheduler_update);
    setupHooks();


    MH_EnableHook(MH_ALL_HOOKS);
    
    return 0;
}



BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x100, thread_func, handle, 0, 0);
        
    }
    return TRUE;
}