#include "LightApp.h"

bool LightApp::Initialize()
{
    if (!D3dApp::Initialize())
    {
        return false;
    }

    

    return true;
}

void LightApp::Update(const GameTimer& InGameTime)
{
    D3dApp::Update(InGameTime);
}

void LightApp::Draw(const GameTimer& InGameTime)
{
}
