#include "INIT.h"

void A5Ext::Init()
{
    // Your extender initialization goes here. The proxy calls this export once
    // per process at the UPC_Init boundary, outside this DLL's DllMain.
}

extern "C" A5EXT_BOOTSTRAP_API DWORD __cdecl Anno5Extender_Init() noexcept
{
    try {
        A5Ext::Init();
        return ERROR_SUCCESS;
    }
    catch (...) {
        return ERROR_UNHANDLED_EXCEPTION;
    }
}
