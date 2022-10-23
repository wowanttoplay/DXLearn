#pragma once
#pragma  comment(lib, "d3dcompiler.lib")
#pragma  comment(lib, "D3D12.lib")
#pragma  comment(lib, "dxgi.lib")

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string>

#include "D3dUtil.h"



class D3dApp
{
public:
    explicit D3dApp(HINSTANCE hInstance);
    D3dApp(const D3dApp&) = delete;
    D3dApp& operator=(const D3dApp&) = delete;
    virtual ~D3dApp();

public:
    int Run();
    
    static D3dApp* GetApp();

    HINSTANCE GetAppInstance() const;
    HWND GetMainWnd() const;
    float GetAspectRatio() const;

    virtual bool Initialize();
    virtual LRESULT MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lparam);

protected:
    bool InitMainWindow();

protected:
    static D3dApp* mApp;

    HINSTANCE mhAppInst = nullptr; // Application instance handle
    HWND mhMainWnd = nullptr; // Main window handle
    std::wstring mMainWndCaption = L"d3d App";
    long mClinetWidth = 800, mClinetHeight = 600;
};
