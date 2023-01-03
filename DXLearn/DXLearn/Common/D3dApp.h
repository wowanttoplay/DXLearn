#pragma once
#include "BaseWindow.h"
#include "GameTimer.h"
#include "MathHelper.h"

class D3dApp : public BaseWindow
{
public:
    D3dApp(HINSTANCE hInsatnce);

public:
    virtual bool Initialize() override;
    int Run() override;
    
    virtual void Update(const GameTimer& InGameTime);
    virtual void Draw(const GameTimer& InGameTime) = 0;
    virtual LRESULT MSgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y);
    virtual void OnMouseUp(WPARAM btnState, int x, int y);  
    virtual void OnMouseMove(WPARAM btnState, int x, int y);

public:
    float GetAspectRatio() const;
    void UpdateCamera();

protected:
    // Init direct 3d
    bool InitDirect3D();
    void BuildDXGIFactory();
    void BuildD3DDevice();
    void BuildFence();
    void InitDescHandleSize();
    void InitMsaaState();
    void CreateCommandObjects(); // Command queue, Command list , Command allocator
    void CreateSwapChain();
    void CreateRtvAndDsvDescHeap();
    
    void LogAdapters() const; // show all display adapter
    void LogAdapterOutputs(IDXGIAdapter* adapter) const;
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) const;
    void ResetSwapChain();
    void CreateRenderTargetBufferAndView();
    void CreateDepthStencilBufferAndView();

    //
    virtual void OnResize();
    void SetMsaaState(bool InState);
    void FlushCommandQueue();
    void ExecuteCommandList() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView() const;
    ID3D12Resource* CurrentRenderTargetBuffer() const;
    

protected:
    Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> mD3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    bool m4xMsaaState = false; // true to use MSAA
    UINT m4xMsaaQuality = 0;   // quality level of msaa
    
    UINT mRtvDescHandleSize = 0;
    UINT mDscDescHandleSize = 0;
    UINT mCbvHandleSize = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAlloctor;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    static constexpr int mSwapChainBufferNumber  = 2;
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[mSwapChainBufferNumber];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
    uint8_t mCurrentSwapChainIndex = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

protected:
    void CalculateFrameState() const;
protected:
    GameTimer mTimer;
    bool mAppPaused; // is the application paused
    bool mMinimized; // is the application minimized?
    bool mMaximized; // is the application maximized?
    bool mResizing = false; // are the resize bars being dragged
    bool mFullScreenState = false; // full screen enabled

protected:
    DirectX::XMFLOAT3 mEyePostion = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mNearZ = 1.0f;
    float mFarZ = 1000.0f;

    float mTheta = 1.5f * DirectX::XM_PI;
    float mPhi = DirectX::XM_PIDIV4;
    float mRadius = 100.0f;

    POINT mLastMousePos;
};
