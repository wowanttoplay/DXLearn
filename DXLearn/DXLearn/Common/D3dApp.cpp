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

    BuildDXGIFactory();

    BuildD3DDevice();

    return true;
}

void D3dApp::BuildDXGIFactory()
{
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory)));
}

void D3dApp::BuildD3DDevice()
{
    // Try to create hardware devcie
    HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mD3dDevice));
    // Fallback to WARP devcie
    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&mDxgiAdapter)));
        ThrowIfFailed(D3D12CreateDevice(mDxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mD3dDevice)));
    }

    
}
