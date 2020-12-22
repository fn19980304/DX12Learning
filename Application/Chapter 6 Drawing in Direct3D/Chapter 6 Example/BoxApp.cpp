/*
*  ��������ʾ����
*  BoxApp.cpp
* 
*  ���ƣ�
*		 �����������϶�����ת
*		 ��������Ҽ��϶�������
* 
*  ���ߣ� Frank Luna (C)
*/

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;


// ����ṹ�壬�����������ɫ
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

// �þ������ڽ������ɾֲ��ռ�任����βü��ռ�
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

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
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

	// ���������б�Ϊִ�г�ʼ����������׼������
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	
}

// �����������ѣ�����ֻ�ô���CBV�������ѣ�
void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;      // ������CBV��SRV��UAV
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  // ����ɫ���������
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&cbvHeapDesc, 
		IID_PPV_ARGS(&mCbvHeap)));
}

// ��������������
void BoxApp::BuildConstantBuffers()
{
	// �����˴洢��һ��ObjectConstants���͵ĳ��������������飩
	// �û������洢�˻���n����������ĳ������ݣ����ڱ���ֻ��Ҫ����һ�������壬n = 1
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	// ��256B��������Ϊ���������
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// ����������ʼ��ַ��������Ϊ0���Ǹ������������ĵ�ַ��
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	
	// ƫ�Ƶ������������е�i����������Ӧ�ĳ������ݣ�����ȡi = 0
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	// cbvDesc�������ǰ󶨵�HLSL�����������ṹ��ĳ�����������Դ�Ӽ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

// ������ǩ��
void BoxApp::BuildRootSignature()
{
	// ��ɫ������һ����Ҫ����Դ��Ϊ���루�����������������������ȣ�
	// ��ǩ����������ɫ����������Ҫ�ľ�����Դ
	// �������ɫ��������һ�������������������Դ�����������ݵĲ�������
	// ��ô������Ƶ���Ϊ��ǩ��������Ǻ���ǩ��

	// �����������������������������������
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// �����ɵ���CBV����ɵ���������
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  // ������������� 
		1,                                // ����������������
		0);                               // ��������������������ַ��ɫ���Ĵ���

	// �����������ʼ��������
	slotRootParameter[0].InitAsDescriptorTable(
		1,           // ��������������
		&cbvTable);  // ָ�����������������ָ�� 

	// ��ǩ����һ�����������
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// �Ƚ���ǩ�����������ֽ������л�����
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	// ��������һ����λ���ò�λָ��һ�����ɵ���������������ɵ����������򣬼�cbvTable���ĸ�ǩ��
	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

// ������ɫ���ֽ�������벼������
void BoxApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	// ������ɫ����������ɫ��
	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	// �������벼������������
	// ����, ���ӵ������ϵ�����, Ԫ�ظ�ʽ���������ͣ�, �����, ƫ����, �߼�������ѡ��, �߼�������ѡ��
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}