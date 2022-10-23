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
        ThrowIfFailed(mDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3dDevice)));
    }
#ifdef _DEBUG
    LogAdapters();
#endif
    
}

void D3dApp::LogAdapters() const
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapters;
    // enum display adapter
    while (mDxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring info = TEXT("*** Adapter: ");
        info += desc.Description;
        info += TEXT("\n");

        OutputDebugString(info.c_str());
        adapters.push_back(adapter);
        ++i;
    }

    // enum display adapter output
    for (size_t adapterIndex = 0; adapterIndex < adapters.size(); ++adapterIndex)
    {
        LogAdapterOutputs(adapters.at(adapterIndex));
        ReleaseCom(adapters.at(adapterIndex));
    }
}

void D3dApp::LogAdapterOutputs(IDXGIAdapter* adapter) const
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring info = TEXT("*** Output:   ");
        info += desc.DeviceName;
        info += TEXT("\n");
        OutputDebugString(info.c_str());

        LogOutputDisplayModes(output, mBackBufferFormat);
        ReleaseCom(output);
        ++i;
    }
}

void D3dApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) const
{
    UINT count = 0;
    UINT flags = 0;
    // Call with nullptr to get list count
    output->GetDisplayModeList(format, flags, &count, nullptr);
    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (const auto& mode : modeList)
    {
        UINT n = mode.RefreshRate.Numerator;
        UINT d = mode.RefreshRate.Denominator;
        std::wstring info = TEXT("Width : ") + std::to_wstring(mode.Width) + TEXT("\t") +
            TEXT("Height : ") + std::to_wstring(mode.Height) + TEXT("\t") +
                TEXT("Refresh : ") + std::to_wstring(n) + TEXT("/") + std::to_wstring(d) +
                    TEXT("\n");
        OutputDebugString(info.c_str());
    }
}
