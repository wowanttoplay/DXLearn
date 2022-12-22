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



class BaseWindow
{
public:
    BaseWindow(HINSTANCE hInstance);
    BaseWindow(const BaseWindow&) = delete;
    BaseWindow& operator=(const BaseWindow&) = delete;
    virtual ~BaseWindow();

public:
    
    
    static BaseWindow* GetApp();
    HINSTANCE GetAppInstance() const;
    HWND GetMainWnd() const;
    
    virtual bool Initialize();
    virtual int Run();
    virtual LRESULT MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lparam);

protected:
    bool InitMainWindow();

protected:
    static BaseWindow* mApp;

    HINSTANCE mhAppInst = nullptr; // Application instance handle
    HWND mhMainWnd = nullptr; // Main window handle
    std::wstring mMainWndCaption = TEXT("BaseWindow");
    long mClientWidth = 800, mClientHeight = 600;
};
