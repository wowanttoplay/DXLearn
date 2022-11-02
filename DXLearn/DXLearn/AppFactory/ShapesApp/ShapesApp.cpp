#include "ShapesApp.h"

bool ShapesApp::Initialize()
{
    if (!D3dApp::Initialize())
    {
        return false;
    }

    return true;
}
