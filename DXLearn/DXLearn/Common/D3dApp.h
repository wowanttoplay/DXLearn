#pragma once
#include "BaseWindow.h"

class D3dApp : public BaseWindow
{
public:
    D3dApp(HINSTANCE hInsatnce);

public:
    virtual bool Initialize() override;
    virtual int Run() override;
    virtual LRESULT MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lparam) override;


public:
    float GetAspectRatio() const;

protected:
    bool InitDirect3D();
    void BuildDXGIFactory();
    
    void BuildD3DDevice();
    void LogAdapters() const; // show all display adapter
    void LogAdapterOutputs(IDXGIAdapter* adapter) const;
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) const;
    

protected:
    Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> mD3dDevice;

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
};
