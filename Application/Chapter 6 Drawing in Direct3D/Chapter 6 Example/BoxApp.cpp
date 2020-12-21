/*
*  立方体演示程序
*  BoxApp.cpp
* 
*  控制：
*		 按下鼠标左键拖动以旋转
*		 按下鼠标右键拖动以缩放
* 
*  作者： Frank Luna (C)
*/

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;


// 顶点结构体，包括坐标和颜色
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

// 该矩阵用于将坐标由局部空间变换到齐次裁剪空间
struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class BoxApp :public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize()override;

private:

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:

	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
}; 

BoxApp::BoxApp(HINSTANCE hInstance) :D3DApp(hInstance)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 重置命令列表为执行初始化命令做好准备工作
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	
}

// 创建描述符堆（本章只用创建CBV描述符堆）
void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;      // 适用于CBV、SRV及UAV
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  // 供着色器程序访问
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&cbvHeapDesc, 
		IID_PPV_ARGS(&mCbvHeap)));
}

// 创建常量缓冲区
void BoxApp::BuildConstantBuffers()
{
	// 创建了存储了一个ObjectConstants类型的常量缓冲区（数组）
	// 该缓冲区存储了绘制n个物体所需的常量数据，由于本章只需要绘制一个立方体，n = 1
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	// 以256B的整数倍为其填充数据
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// 缓冲区的起始地址（即索引为0的那个常量缓冲区的地址）
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	
	// 偏移到常量缓冲区中第i个物体所对应的常量数据，这里取i = 0
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	// cbvDesc描述的是绑定到HLSL常量缓冲区结构体的常量缓冲区资源子集
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}