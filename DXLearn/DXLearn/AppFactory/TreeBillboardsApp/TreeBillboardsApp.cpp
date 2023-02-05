#include "TreeBillboardsApp.h"

#include <iostream>

#include "../../Common/DDSTextureLoader.h"
#include "../../Common/FileManager.h"
using namespace DirectX;


TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInsatnce)
    : BlendApp(hInsatnce)
{
    // Registe initialzie functions
}

void TreeBillboardsApp::BuildTextures()
{
    BlendApp::BuildTextures();

    auto treeArrayTex = std::make_unique<Texture>();
    treeArrayTex->Name = "treeArrayTex";
    treeArrayTex->Filename = FileManager::GetTextureFullPath("treeArray2.dds");
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mD3dDevice.Get(),
        mCommandList.Get(), treeArrayTex->Filename.c_str(), treeArrayTex->Resource, treeArrayTex->UploadHeap));

    mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
}



