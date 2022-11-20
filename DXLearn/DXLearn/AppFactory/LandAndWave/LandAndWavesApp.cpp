#include "LandAndWavesApp.h"

#include <DirectXColors.h>

#include "LWFrameResource.h"
#include "../../Common/GeometryGenerator.h"

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
   
   BuildRootSignature();
   BuildShadersAndInputLayout();
   BuildLandGeometry();
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
}

void LandAndWavesApp::Update(const GameTimer& InGameTime)
{
   D3dApp::Update(InGameTime);
}

void LandAndWavesApp::BuildRootSignature()
{
}

void LandAndWavesApp::BuildShadersAndInputLayout()
{
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

   geo->DrawArgs["Land"] = LandSubMesh;
   mGeometries = std::move(geo);
}

void LandAndWavesApp::BuildRenderItem()
{
}

void LandAndWavesApp::BuildFrameResource()
{
}

void LandAndWavesApp::BuildDescriptor()
{
}

void LandAndWavesApp::BuildConstantBufferView()
{
}

void LandAndWavesApp::BuildPSO()
{
}


