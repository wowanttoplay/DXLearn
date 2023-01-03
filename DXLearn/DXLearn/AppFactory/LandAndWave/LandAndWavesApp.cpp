#include "LandAndWavesApp.h"

#include <DirectXColors.h>
#include <fstream>

#include "LWFrameResource.h"
#include "../../Common/GeometryGenerator.h"

using namespace DirectX;

float GetHillsHeight(float x, float z)
{
   return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

bool LandAndWavesApp::Initialize()
{
   if (!D3dApp::Initialize())
   {
      return false;
   }
   ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), nullptr));

   mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
   
   BuildRootSignature();
   BuildShadersAndInputLayout();
   
   BuildLandGeometry();
   BuildWaveGeomtry();
   BuildRenderItem();
   
   BuildFrameResource();
   
   BuildPSO();

   ThrowIfFailed(mCommandList->Close());
   ID3D12CommandList* cmdList[] = {mCommandList.Get()};
   mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

   FlushCommandQueue();
   return true;
}

void LandAndWavesApp::Draw(const GameTimer& InGameTime)
{
   auto cmdAlloc = mCurrFrameResource->CmdListAlloc;
   ThrowIfFailed(cmdAlloc->Reset());

   if (mIsWireframe)
   {
      ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
   }
   else
   {
      ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs["opaque"].Get()));
   }

   mCommandList->RSSetViewports(1, &mScreenViewport);
   mCommandList->RSSetScissorRects(1, &mScissorRect);

   mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
   mCommandList->ClearRenderTargetView(RenderTargetView(), DirectX::Colors::Aqua, 0, nullptr);
   mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
   mCommandList->OMSetRenderTargets(1, &RenderTargetView(), true, &DepthStencilView());

   mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

   auto passCB = mCurrFrameResource->PassCB->GetResource();
   mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

   DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)ERenderLayer::Opaque]);

   mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
   ThrowIfFailed(mCommandList->Close());
   ID3D12CommandList* cmdList[] = {mCommandList.Get()};
   mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

   ThrowIfFailed(mSwapChain->Present(0,0));
   mCurrentSwapChainIndex = (mCurrentSwapChainIndex + 1) % mSwapChainBufferNumber;

   mCurrFrameResource->Fence = ++mCurrentFence;
   mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void LandAndWavesApp::Update(const GameTimer& InGameTime)
{
   D3dApp::Update(InGameTime);

   OnKeyboardInput(InGameTime);

   mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
   mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

   // Has the GPU finished processing the commands of the current frame resource?
   // If not, wait until the GPU has completed commands up to this fence point.
   if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
   {
      HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
      ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
      WaitForSingleObject(eventHandle, INFINITE);
      CloseHandle(eventHandle);
   }
   UpdateObjectCBs(InGameTime);
   UpdateMainPassCB(InGameTime);
   UpdateWaves(InGameTime);
   
}

void LandAndWavesApp::BuildRootSignature()
{
   CD3DX12_ROOT_PARAMETER slotRootParameter[2];
   // Create root constant buffer view
   slotRootParameter[0].InitAsConstantBufferView(0);
   slotRootParameter[1].InitAsConstantBufferView(1);
   // A root signature is an array of root parameters.
   CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

   // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
   Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
   Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
   HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
       serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

   if(errorBlob != nullptr)
   {
      ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
   }
   ThrowIfFailed(hr);

   ThrowIfFailed(mD3dDevice->CreateRootSignature(
      0,
      serializedRootSig->GetBufferPointer(),
      serializedRootSig->GetBufferSize(),
      IID_PPV_ARGS(mRootSignature.GetAddressOf())
   ));
}

void LandAndWavesApp::BuildShadersAndInputLayout()
{
   const std::wstring ShaderPath = TEXT("AppFactory\\Shaders\\color.hlsl");

   mShaders["standardVS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "VS", "vs_5_1");
   mShaders["opaquePS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "PS", "ps_5_1");

   mInputLayout =
   {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
}

void LandAndWavesApp::BuildLandGeometry()
{
   GeometryGenerator geoGen;
   GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.f,160.f,50, 50);
   // calculate the height of land, set the color by height to differentiate terrains
   std::vector<LWVertex> vertices(grid.Vertices.size());
   for (size_t i = 0; i < grid.Vertices.size(); ++i)
   {
      auto& pos = grid.Vertices[i].Position;
      vertices[i].Pos = pos;
      vertices[i].Pos.y = GetHillsHeight(pos.x, pos.z);

      // Color the vertex based on it's height
      constexpr DirectX::XMFLOAT4 SandBeachColor(1.0f, 0.96f, 0.62f, 1.0f);
      constexpr DirectX::XMFLOAT4 BrightYellowGreen(0.48f, 0.77f, 0.46f, 1.0f);
      constexpr DirectX::XMFLOAT4 DarkYellowGreen(0.1f, 0.48f, 0.19f, 1.0f);
      constexpr DirectX::XMFLOAT4 DarkBrown(0.45f, 0.39f, 0.34f, 1.0f);
      constexpr DirectX::XMFLOAT4 WhiteSnow(1.0f, 1.0f, 1.0f, 1.0f);

      if (vertices[i].Pos.y < -10.0f)
      {
         vertices[i].Color = SandBeachColor;
      }
      else if (vertices[i].Pos.y < 5.0f)
      {
         vertices[i].Color = BrightYellowGreen;
      }
      else if (vertices[i].Pos.y < 12.0f)
      {
         vertices[i].Color = DarkYellowGreen;
      }
      else if (vertices[i].Pos.y < 20.0f)
      {
         vertices[i].Color = DarkBrown;
      }
      else
      {
         vertices[i].Color = WhiteSnow;
      }
   }

   const UINT vbByteSize = sizeof(LWVertex) * static_cast<UINT>(vertices.size());
   std::vector<uint16_t> indices = grid.GetIndices16();
   const UINT ibByteSize = sizeof(uint16_t) * static_cast<UINT>(indices.size());

   auto geo = std::make_unique<MeshGeometry>();
   geo->Name = "LandGeo";

   ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
   CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
   ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
   CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

   geo->VertexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
   geo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

   geo->VertexByteStride = sizeof(LWVertex);
   geo->VertexBufferByteSize = vbByteSize;
   geo->IndexFormat = DXGI_FORMAT_R16_UINT;
   geo->IndexBufferByteSize = ibByteSize;

   SubMeshGeometry LandSubMesh;
   LandSubMesh.IndexCount = static_cast<UINT>(indices.size());
   LandSubMesh.BaseVertexLocation = 0;
   LandSubMesh.StartIndexLocation = 0;

   geo->DrawArgs["grid"] = LandSubMesh;

   mGeometries["landGeo"] = std::move(geo);
}

void LandAndWavesApp::BuildWaveGeomtry()
{
   std::vector<uint16_t> indices(3 * mWaves->TriangleCount());
   assert(mWaves->VertexCount() < 0x0000ffff);

   int m = mWaves->RowCount();
   int n = mWaves->ColumnCount();
   int k = 0;
   for(int i = 0; i < m - 1; ++i)
   {
      for(int j = 0; j < n - 1; ++j)
      {
         indices[k] = i*n + j;
         indices[k + 1] = i*n + j + 1;
         indices[k + 2] = (i + 1)*n + j;

         indices[k + 3] = (i + 1)*n + j;
         indices[k + 4] = i*n + j + 1;
         indices[k + 5] = (i + 1)*n + j + 1;

         k += 6; // next quad
      }
   }

   UINT vbByteSize = mWaves->VertexCount()*sizeof(LWVertex);
   UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);

   auto geo = std::make_unique<MeshGeometry>();
   geo->Name = "waterGeo";

   // Set dynamically.
   geo->VertexBufferCPU = nullptr;
   geo->VertexBufferGPU = nullptr;

   ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
   CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

   geo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(),
      mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

   geo->VertexByteStride = sizeof(LWVertex);
   geo->VertexBufferByteSize = vbByteSize;
   geo->IndexFormat = DXGI_FORMAT_R16_UINT;
   geo->IndexBufferByteSize = ibByteSize;

   SubMeshGeometry submesh;
   submesh.IndexCount = (UINT)indices.size();
   submesh.StartIndexLocation = 0;
   submesh.BaseVertexLocation = 0;

   geo->DrawArgs["grid"] = submesh;

   mGeometries["waterGeo"] = std::move(geo);
}

void LandAndWavesApp::BuildRenderItem()
{
   // Wave
   auto wavesRenderItem = std::make_unique<RenderItem>();
   wavesRenderItem->World = MathHelper::Identity4x4();
   wavesRenderItem->ObjCBIndex = 0;
   wavesRenderItem->Geo = mGeometries["waterGeo"].get();
   wavesRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   const auto& waveSubMesh = wavesRenderItem->Geo->DrawArgs["grid"];
   wavesRenderItem->IndexCount = waveSubMesh.IndexCount;
   wavesRenderItem->StartIndexLocation = waveSubMesh.StartIndexLocation;
   wavesRenderItem->BaseVertexLocation = waveSubMesh.BaseVertexLocation;
   mWaveRenderItem = wavesRenderItem.get();
   mRitemLayer[static_cast<int>(ERenderLayer::Opaque)].push_back(wavesRenderItem.get());

   // Land
   auto gridRitem = std::make_unique<RenderItem>();
   gridRitem->World = MathHelper::Identity4x4();
   gridRitem->ObjCBIndex = 1;
   gridRitem->Geo = mGeometries["landGeo"].get();
   gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
   gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
   gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

   mRitemLayer[(int)ERenderLayer::Opaque].push_back(gridRitem.get());
   
   mAllRenderItems.push_back(std::move(wavesRenderItem));
   mAllRenderItems.push_back(std::move(gridRitem));
}

void LandAndWavesApp::BuildFrameResource()
{
   for (int i = 0; i < gNumFrameResources; ++i)
   {
      mFrameResources.push_back(std::make_unique<LWFrameResource>(mD3dDevice.Get(), 1, static_cast<UINT>(mAllRenderItems.size()), mWaves->VertexCount()));
   }
}

void LandAndWavesApp::BuildPSO()
{
   D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

   //
   // PSO for opaque objects.
   //
   ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
   opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
   opaquePsoDesc.pRootSignature = mRootSignature.Get();
   opaquePsoDesc.VS =
   {
      reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
      mShaders["standardVS"]->GetBufferSize()
   };
   opaquePsoDesc.PS =
   {
      reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
      mShaders["opaquePS"]->GetBufferSize()
   };
   opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
   opaquePsoDesc.SampleMask = UINT_MAX;
   opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   opaquePsoDesc.NumRenderTargets = 1;
   opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
   opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
   opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
   opaquePsoDesc.DSVFormat = mDepthStencilFormat;
   
   ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

   // PSO for opaque wireframe objects
   D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
   opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
   ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
}

void LandAndWavesApp::OnKeyboardInput(const GameTimer& gt)
{
   if(GetAsyncKeyState('1') & 0x8000)
      mIsWireframe = true;
   else
      mIsWireframe = false;
}

void LandAndWavesApp::UpdateObjectCBs(const GameTimer& game_timer)
{
   auto currObjectCB = mCurrFrameResource->ObjectCB.get();
   for(auto& e : mAllRenderItems)
   {
      // Only update the cbuffer data if the constants have changed.  
      // This needs to be tracked per frame resource.
      if(e->NumFramesDirty > 0)
      {
         DirectX::XMMATRIX world = XMLoadFloat4x4(&e->World);

         LWObjectConstants objConstants;
         XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

         currObjectCB->CopyData(e->ObjCBIndex, objConstants);

         // Next FrameResource need to be updated too.
         e->NumFramesDirty--;
      }
   }
}

void LandAndWavesApp::UpdateMainPassCB(const GameTimer& game_timer)
{
   DirectX::XMMATRIX view = XMLoadFloat4x4(&mView);
   XMMATRIX proj = XMLoadFloat4x4(&mProj);

   XMMATRIX viewProj = XMMatrixMultiply(view, proj);
   XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
   XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
   XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

   XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
   XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
   XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
   XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
   XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
   XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
   mMainPassCB.EyePosW = mEyePostion;
   mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
   mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
   mMainPassCB.NearZ = 1.0f;
   mMainPassCB.FarZ = 1000.0f;
   mMainPassCB.TotalTime = game_timer.TotalTime();
   mMainPassCB.DeltaTime = game_timer.DeltaTime();

   auto currPassCB = mCurrFrameResource->PassCB.get();
   currPassCB->CopyData(0, mMainPassCB);
}

void LandAndWavesApp::UpdateWaves(const GameTimer& game_timer)
{
   // Every quarter second, generate a random wave.
   static float t_base = 0.0f;
   if((mTimer.TotalTime() - t_base) >= 0.25f)
   {
      t_base += 0.25f;

      int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
      int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

      float r = MathHelper::RandF(0.2f, 0.5f);

      mWaves->Disturb(i, j, r);
   }

   // Update the wave simulation.
   mWaves->Update(game_timer.DeltaTime());

   // Update the wave vertex buffer with the new solution.
   auto currWavesVB = mCurrFrameResource->WavesVB.get();
   for(int i = 0; i < mWaves->VertexCount(); ++i)
   {
      LWVertex v;

      v.Pos = mWaves->Position(i);
      v.Color = XMFLOAT4(DirectX::Colors::SkyBlue);

      currWavesVB->CopyData(i, v);
   }

   // Set the dynamic VB of the wave renderitem to the current frame VB.
   mWaveRenderItem->Geo->VertexBufferGPU = currWavesVB->GetResource();
}

void LandAndWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList,
   const std::vector<RenderItem*>& render_items)
{
   UINT objCBByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(LWObjectConstants));

   auto objectCB = mCurrFrameResource->ObjectCB->GetResource();

   // For each render item...
   for(size_t i = 0; i < render_items.size(); ++i)
   {
      auto ri = render_items[i];

      cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
      cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
      cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

      D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
      objCBAddress += ri->ObjCBIndex*objCBByteSize;

      cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

      cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
   }
}


