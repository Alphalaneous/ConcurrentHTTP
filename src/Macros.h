#include <windows.h>
#include <MinHook.h>


#define CreateHook(Addr, Func) MH_CreateHook(reinterpret_cast<void*>(Addr),\
                                                    Func ## _H, \
                                                    reinterpret_cast<void**>(&Func))

#define CCAddr(ID) GetProcAddress(GetModuleHandleA("libcocos2d.dll"), ID)

#define ExtAddr(ID) GetProcAddress(GetModuleHandleA("libExtensions.dll"), ID)