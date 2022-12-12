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
}

void LandAndWavesApp::Update(const GameTimer& InGameTime)
{
   D3dApp::Update(InGameTime);
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
   const std::wstring ShaderPath = TEXT("AppFactory\\ShapesApp\\Shaders\\color.hlsl");

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
   wavesRenderItem->objectIndex = 0;
   wavesRenderItem->Geo = mGeometries["waterGeo"].get();
   wavesRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   const auto& waveSubMesh = wavesRenderItem->Geo->DrawArgs["grid"];
   wavesRenderItem->IndexCount = waveSubMesh.IndexCount;
   wavesRenderItem->StartIndexLocation = waveSubMesh.StartIndexLocation;
   wavesRenderItem->BaseVertexLocation = waveSubMesh.BaseVertexLocation;
   mWaveRenderItem = wavesRenderItem.get();
   mRitemLayer[static_cast<int>(RenderLayer::Opaque)].push_back(wavesRenderItem.get());

   // Land
   auto gridRitem = std::make_unique<RenderItem>();
   gridRitem->World = MathHelper::Identity4x4();
   gridRitem->objectIndex = 1;
   gridRitem->Geo = mGeometries["landGeo"].get();
   gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
   gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
   gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

   mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
   
   mAllRenderItems.push_back(std::move(wavesRenderItem));
   mAllRenderItems.push_back(std::move(gridRitem));
}

void LandAndWavesApp::BuildFrameResource()
{
}

void LandAndWavesApp::BuildPSO()
{
}


