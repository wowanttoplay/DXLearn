
#include <DirectXColors.h>

#include "AppFactory/TreeBillboardsApp/TreeBillboardsApp.h"
#include "Common/BaseWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        TreeBillboardsApp theApp(hInstance);
        if (!theApp.Initialize())
        {
            return 0;
        }
        theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), TEXT("HR Failed"), MB_OK);
        return 0;
    }
    return 0;
}
