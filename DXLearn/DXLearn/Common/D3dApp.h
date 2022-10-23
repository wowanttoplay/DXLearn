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

    //
    virtual void OnResize();
    void FlushCommandQueue();
    

protected:
    Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> mD3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    bool mMsaaState = false; // true to use MSAA
    UINT mMsaaQuality = 0;   // quality level of msaa
    
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

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
};
