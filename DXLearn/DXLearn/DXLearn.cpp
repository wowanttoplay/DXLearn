
#include <DirectXColors.h>
#include "Common/D3dApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
    #if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        D3dApp theApp(hInstance);
        if (!theApp.Initialize())
        {
            return 0;
        }
        theApp.Run();
    }
    catch (...)
    {
    }
}