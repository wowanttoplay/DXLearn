#include "D3dApp.h"

#include <assert.h>

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return D3dApp::GetApp()->MSgProc(hWnd, msg, wParam, lParam);
}

D3dApp* D3dApp::mApp = nullptr;

D3dApp::D3dApp(HINSTANCE hInstance)
    : mhAppInst(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
}

D3dApp::~D3dApp()
{
}

D3dApp* D3dApp::GetApp()
{
    return mApp;
}

HINSTANCE D3dApp::GetAppInstance() const
{
    return  mhAppInst;
}

HWND D3dApp::GetMainWnd() const
{
    return mhMainWnd;
}

float D3dApp::GetAspectRatio() const
{
    return static_cast<float>(mClinetWidth) / mClinetHeight;
}

bool D3dApp::Initialize()
{
    if (InitMainWindow())
    {
        return false;
    }
    return true;
}

int D3dApp::Run()
{
    MSG msg = {0};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}

LRESULT D3dApp::MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_ACTIVATE:
        return 0;
    case WM_SIZE:
        mClinetWidth = LOWORD(lparam);
        mClinetHeight = HIWORD(lparam);
        return 0;
    case WM_ENTERSIZEMOVE:
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_MENUCHAR:
        return MAKELRESULT(0, MNC_CLOSE);
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lparam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lparam)->ptMinTrackSize.y = 200;
        return 0;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lparam);
}

bool D3dApp::InitMainWindow()
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = mhAppInst;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = TEXT("MainWindow");

    if (!RegisterClass(&wc))
    {
        MessageBox(0, TEXT("RegisterClass Failed"), 0, 0);
        return false;
    }

    // Compute window rectangle dimensions based on requested client area dimensions
    RECT R = {0, 0, mClinetWidth, mClinetHeight};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    mhMainWnd = CreateWindow(TEXT("MainWindow"), mMainWndCaption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
    if (!mhMainWnd)
    {
        MessageBox(0, TEXT("CreateWindow Failed"), 0, 0);
        return false;
    }

    ShowWindow(mhMainWnd, SW_SHOW);
    UpdateWindow(mhMainWnd);

    return true;
}
