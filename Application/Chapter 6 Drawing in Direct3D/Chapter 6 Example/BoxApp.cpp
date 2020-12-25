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

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;
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

// ����������
void BoxApp::BuildBoxGeometry()
{
	// ����������İ˸�����
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	// ���������������
	std::array<std::uint16_t, 36> indices =
	{
		// ǰ����
		0, 1, 2,
		0, 2, 3,

		// �����
		4, 6, 5,
		4, 7, 6,

		// �����
		4, 5, 1,
		4, 1, 0,

		// �ұ���
		3, 2, 6,
		3, 6, 7,

		// �ϱ���
		1 ,5, 6,
		1, 6, 2,

		// �±���
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	// ���������ݸ��Ƶ�CPU�ڴ���
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	// ���������ݸ��Ƶ�CPU�ڴ���
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	// ͨ���������ݴ�����Ӧ�Ķ��㻺������Դ
	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	// ͨ���������ݴ�����Ӧ��������������Դ
	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}

// ������ˮ��״̬����
void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };  // ���벼������
	psoDesc.pRootSignature = mRootSignature.Get();                             // ��ǩ��ָ��
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};                                                                         // ������ɫ��
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};											                               // ������ɫ��

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          // ��դ��״̬
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);					   // ���״̬
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     // ���/ģ��״̬
	psoDesc.SampleMask = UINT_MAX;											   // �����е���в���
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;    // ͼԪ����������
	psoDesc.NumRenderTargets = 1;											   // ͬʱ���õ���ȾĿ������
	psoDesc.RTVFormats[0] = mBackBufferFormat;								   // ��ȾĿ���ʽ
	psoDesc.SampleDesc.Count= m4xMsaaState ? 4 : 1;							   // ���ز��ö�ÿ�����صĲ�������
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;      // ���ز�������������
	psoDesc.DSVFormat = mDepthStencilFormat;                                   // ���/ģ�建�����ĸ�ʽ
	
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}