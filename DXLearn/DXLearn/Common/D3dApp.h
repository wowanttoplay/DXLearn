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

protected:
    Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
    Microsoft::WRL::ComPtr<IDXGIDevice> mD3dDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter> mDxgiAdapter;
};
