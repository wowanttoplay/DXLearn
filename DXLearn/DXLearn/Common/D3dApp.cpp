#include "D3dApp.h"
#include <windowsx.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

D3dApp::D3dApp(HINSTANCE hInsatnce) :
BaseWindow(hInsatnce)
{
}

bool D3dApp::Initialize()
{
    if (!BaseWindow::Initialize())
    {
        return false;
    }

    if (!InitDirect3D())
    {
        return false;
    }

    return true;
}

int D3dApp::Run()
{
    return BaseWindow::Run();
}

LRESULT D3dApp::MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lparam)
{
    return BaseWindow::MSgProc(hWnd, msg, wParam, lparam);
}

float D3dApp::GetAspectRatio() const
{
    return static_cast<float>(mClinetWidth) / mClinetHeight;
}

bool D3dApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    return true;
}
