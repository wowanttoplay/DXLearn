#include "BaseApp.h"

#include <DirectXColors.h>

#include "../Common/d3dx12.h"

void BaseApp::Update(const GameTimer& InGameTime)
{
    
}

void BaseApp::Draw(const GameTimer& InGameTime)
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(mCommandAlloctor->Reset());
    ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), nullptr));

    // Indicate a state transition on the resource usage from present to render target
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // set the viewport and scissor rect
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // clear the render target buffer and depth stencil buffer
    mCommandList->ClearRenderTargetView(RenderTargetView(), DirectX::Colors::LightSkyBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

    // Specify the buffers we are going to render to
    mCommandList->OMSetRenderTargets(1, &RenderTargetView(), true, &DepthStencilView());

    // Indicate a state transition on the resource usage
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdList[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    // swap the back and front buffer
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentSwapChainIndex = (mCurrentSwapChainIndex + 1) % mSwapChainBufferNumber;

    FlushCommandQueue();

    
}
