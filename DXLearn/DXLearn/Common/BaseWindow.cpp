#include "BaseWindow.h"

#include <assert.h>

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return BaseWindow::GetApp()->MSgProc(hWnd, msg, wParam, lParam);
}

BaseWindow* BaseWindow::mApp = nullptr;

BaseWindow::BaseWindow(HINSTANCE hInstance)
    : mhAppInst(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
}

BaseWindow::~BaseWindow()
{
}

BaseWindow* BaseWindow::GetApp()
{
    return mApp;
}

HINSTANCE BaseWindow::GetAppInstance() const
{
    return  mhAppInst;
}

HWND BaseWindow::GetMainWnd() const
{
    return mhMainWnd;
}

bool BaseWindow::Initialize()
{
    if (!InitMainWindow())
    {
        return false;
    }
    return true;
}

int BaseWindow::Run()
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

LRESULT BaseWindow::MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lparam)
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
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lparam);
}

bool BaseWindow::InitMainWindow()
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
