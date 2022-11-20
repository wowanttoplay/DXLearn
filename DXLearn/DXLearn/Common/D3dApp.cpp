#include "D3dApp.h"
#include <windowsx.h>

#include "d3dx12.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

D3dApp::D3dApp(HINSTANCE hInsatnce) :
BaseWindow(hInsatnce)
{
    mMainWndCaption = TEXT("D3dApp");
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

    OnResize();

    return true;
}

int D3dApp::Run()
{
   MSG msg = {0};

    mTimer.Reset();
    
   while (msg.message != WM_QUIT)
   {
       if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
       {
           TranslateMessage(&msg);
           DispatchMessage(&msg);
       }
       else
       {
           mTimer.Tick();

           if (!mAppPaused)
           {
               CalculateFrameState();
               Update(mTimer);
               Draw(mTimer);
           }
           else
           {
               Sleep(100);
           }
       }
   }

    return static_cast<int>(msg.wParam);
}


void D3dApp::Update(const GameTimer& InGameTime)
{
    UpdateCamera();
}

LRESULT D3dApp::MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // WM_ACTIVATE is sent when the window is activated or deactivated.  
        // We pause the game when the window is deactivated and unpause it 
        // when it becomes active. 
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            mAppPaused = true;
            mTimer.Stop();
        }
        else
        {
            mAppPaused = false;
            mTimer.Start();
        }
        return 0;
        // WM_SIZE is sent when the user resizes the window. 
    case WM_SIZE:
        mClinetWidth = LOWORD(lParam);
        mClinetHeight = HIWORD(lParam);
        if (mD3dDevice)
        {
            if (wParam == SIZE_MINIMIZED)
            {
                mAppPaused = true;
                mMinimized = true;
                mMaximized = false;
            }
            else if (wParam == SIZE_MAXIMIZED)
            {
                mAppPaused = false;
                mMinimized = false;
                mMaximized = true;
            }
            else if (wParam == SIZE_RESTORED)
            {
                if (mMinimized)
                {
                    mAppPaused = false;
                    mMinimized = false;
                    OnResize();
                }
                else if (mMaximized)
                {
                    mAppPaused = false;
                    mMaximized = false;
                    OnResize();
                }
                else if (mResizing)
                {
                    
                }
                else
                {
                    OnResize();
                }
            }
        }
        return 0;
    case WM_ENTERSIZEMOVE:
        mAppPaused = true;
        mResizing = true;
        mTimer.Stop();
        return 0;
    case WM_EXITSIZEMOVE:
        mAppPaused = false;
        mResizing = false;
        mTimer.Start();
        OnResize();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        // The WM_MENUCHAR message is sent when a menu is active and the user presses 
        // a key that does not correspond to any mnemonic or accelerator key. 
    case WM_MENUCHAR:
        return MAKELRESULT(0, MNC_CLOSE);
        // Catch this message so to prevent the window from becoming too small.
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
        return 0;
        
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            SetMsaaState(!mMsaaState);

        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void D3dApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void D3dApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void D3dApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 30.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

float D3dApp::GetAspectRatio() const
{
    return static_cast<float>(mClinetWidth) / mClinetHeight;
}

void D3dApp::UpdateCamera()
{
    // Convert Spherical to Cartesian coordinates.
    mEyePso.x = mRadius*sinf(mPhi)*cosf(mTheta);
    mEyePso.z = mRadius*sinf(mPhi)*sinf(mTheta);
    mEyePso.y = mRadius*cosf(mPhi);

    // Build the view matrix.
    XMVECTOR pos = XMVectorSet(mEyePso.x, mEyePso.y, mEyePso.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);
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

    BuildFence();

    InitDescHandleSize();

    InitMsaaState();

    CreateCommandObjects();

    CreateSwapChain();

    CreateRtvAndDsvDescHeap();

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

void D3dApp::BuildFence()
{
    ThrowIfFailed(mD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void D3dApp::InitDescHandleSize()
{
    mRtvDescHandleSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDscDescHandleSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvHandleSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3dApp::InitMsaaState()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;

    ThrowIfFailed(mD3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

    mMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(mMsaaQuality > 0);
}

void D3dApp::CreateCommandObjects()
{
    // create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(mD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    // create command allocator
    ThrowIfFailed(mD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAlloctor)));

    // create command list
    ThrowIfFailed(mD3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAlloctor.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));

    // Close command list
    mCommandList->Close();
}

void D3dApp::CreateSwapChain()
{
    // Release the previous swapchain will be recreating
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    swapChainDesc.BufferDesc.Width = mClinetWidth;
    swapChainDesc.BufferDesc.Height = mClinetHeight;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 120;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = mBackBufferFormat;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = mMsaaState ? 4 : 1;
    swapChainDesc.SampleDesc.Quality = mMsaaState ? mMsaaQuality - 1 : 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = mhMainWnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    // create swap chain, swap chain need command queue to perform flush
    ThrowIfFailed(mDxgiFactory->CreateSwapChain(mCommandQueue.Get(), &swapChainDesc, &mSwapChain));
}

void D3dApp::CreateRtvAndDsvDescHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = mSwapChainBufferNumber;
    rtvHeapDesc.NodeMask = 0;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc;
    DsvHeapDesc.NumDescriptors = 1;
    DsvHeapDesc.NodeMask = 0;
    DsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
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

void D3dApp::ResetSwapChain()
{
    // Release the previous resource we will be recreating
    for (int SwapChainIndex = 0; SwapChainIndex < mSwapChainBufferNumber; ++SwapChainIndex)
    {
        mSwapChainBuffer[SwapChainIndex].Reset();
    }
    mDepthStencilBuffer.Reset();

    // Resize the swap chain
    ThrowIfFailed(mSwapChain->ResizeBuffers(mSwapChainBufferNumber, mClinetWidth, mClinetHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    mCurrentSwapChainIndex = 0;
}

void D3dApp::CreateRenderTargetBufferAndView()
{
    // create render target view,use swap chain buffer as render target buffer,dimension
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeaphandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT index = 0; index < mSwapChainBufferNumber; ++index)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(index, IID_PPV_ARGS(&mSwapChainBuffer[index])));
        mD3dDevice->CreateRenderTargetView(mSwapChainBuffer[index].Get(), nullptr, rtvHeaphandle);
        rtvHeaphandle.Offset(1, mRtvDescHandleSize);
    }
}

void D3dApp::CreateDepthStencilBufferAndView()
{
    // create the depth stencil buffer
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClinetWidth;
    depthStencilDesc.Height = mClinetHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = mMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = mMsaaState ? mMsaaQuality - 1 : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    ThrowIfFailed(mD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    // create the depth stencil view
    mD3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());
}

void D3dApp::OnResize()
{
    assert(mDxgiFactory);
    assert(mD3dDevice);
    assert(mSwapChain);
    assert(mCommandQueue);
    assert(mCommandAlloctor);
    assert(mCommandList);

    // Flush before changing any resource
    FlushCommandQueue();

    ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), nullptr));

    ResetSwapChain();

    CreateRenderTargetBufferAndView();
    
    CreateDepthStencilBufferAndView();
    
    // Transition the resource state from common to depth write
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
         D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    // Excute the resize command
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);
    FlushCommandQueue();
    
    // Update the viewport transform to cover the clinet area
    mViewport.TopLeftX = 0;
    mViewport.TopLeftY = 0;
    mViewport.Width = static_cast<float>(mClinetWidth);
    mViewport.Height = static_cast<float>(mClinetHeight);
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;

    mScissorRect = {0, 0, mClinetWidth, mClinetHeight};

    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, proj);
}

void D3dApp::SetMsaaState(bool InState)
{
    if (mMsaaState != InState)
    {
        mMsaaState = InState;
        CreateSwapChain();
        OnResize();
    }
}

void D3dApp::FlushCommandQueue()
{
    // Advance the fence value to mark command up to this fence point
    mCurrentFence += 1;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

    // Wait until the GPU has completed commands up to the fence point
    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

        // wait until the gpu hits current fence event
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE D3dApp::DepthStencilView() const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3dApp::RenderTargetView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrentSwapChainIndex,
        mRtvDescHandleSize
    );
}

ID3D12Resource* D3dApp::CurrentRenderTargetBuffer() const
{
    return mSwapChainBuffer[mCurrentSwapChainIndex].Get();
}

void D3dApp::CalculateFrameState() const
{
    static int frameCount = 0;
    static float timeElapsed = 0.0f;

    frameCount += 1;

    //Compute averages over one second period
    if (mTimer.TotalTime() - timeElapsed >= 1.0f)
    {
        const auto fps = static_cast<float>(frameCount); // fps = frameCount / 1;
        const float mspf = 1000.0f / fps;

        const wstring fpsStr = to_wstring(fps);
        const wstring mspfStr = to_wstring(mspf);

        const wstring windowText = mMainWndCaption + TEXT("\tfps: ") + fpsStr + TEXT("\tmspf: ") + mspfStr;

        SetWindowText(mhMainWnd, windowText.c_str());

        // Reset fro the next seconds
        frameCount = 0;
        timeElapsed += 1.0f;
    }
}
