/*
*  ��6��ϰ��4
*  Exercise4.cpp
*
*  ���ƣ�
*		 �����������϶�����ת
*		 ��������Ҽ��϶�������
*
*  ���ߣ� fn19980304
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

class PyramidApp :public D3DApp
{
public:
	PyramidApp(HINSTANCE hInstance);
	PyramidApp(const PyramidApp& rhs) = delete;
	PyramidApp& operator=(const PyramidApp& rhs) = delete;
	~PyramidApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPyramidGeometry();
	void BuildPSO();

private:

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	std::unique_ptr<MeshGeometry> mPyramidGeo = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ������ʼ�����꣬ʹ��ʼ�ӽ���ͼ6.8��ʾ
	float mTheta = -0.1f * XM_PI;
	float mPhi = 1.6f * XM_PIDIV4;
	float mRadius = 6.0f;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// ��Ե��԰汾��������ʱ�ڴ���
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		PyramidApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

PyramidApp::PyramidApp(HINSTANCE hInstance) :D3DApp(hInstance)
{
}

PyramidApp::~PyramidApp()
{
}

bool PyramidApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// ���������б�Ϊִ�г�ʼ����������׼������
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();		   // ������������
	BuildConstantBuffers();		   // ��������������
	BuildRootSignature();          // ������ǩ��
	BuildShadersAndInputLayout();  // ������ɫ�����򡢹������벼������
	BuildPyramidGeometry();        // ��������׶
	BuildPSO();                    // ������ˮ��״̬����

	// ִ�г�ʼ������
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ȴ���ʼ�����
	FlushCommandQueue();

	return true;
}

void PyramidApp::OnResize()
{
	D3DApp::OnResize();

	// ���û������˴��ڳߴ磬������ݺ�Ȳ����¼���ͶӰ����
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25 * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void PyramidApp::Update(const GameTimer& gt)
{
	// ��������ת��Ϊ�ѿ�������
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// �����۲����
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	// �õ�ǰ���µ�worldViewProj���������³���������
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));  // Ϊ��Ҫת�ã�
	mObjectCB->CopyData(0, objConstants);
}

void PyramidApp::Draw(const GameTimer& gt)
{
	// ���ü�¼�������õ��ڴ�
	// ֻ�е�GPU�е������б�ִ����Ϻ����ǲſ��Զ����������
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// ͨ������ExecuteCommandList�������б����������к󣬱�ɶ�����������
	// ���������б���������Ӧ���ڴ�
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	// �����ӿںͲü�����
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// ������Դ����;ָʾ��״̬��ת�䣬�˴�����Դ�ӳ���״̬ת��Ϊ��ȾĿ��״̬
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// �����̨����������Ȼ�����
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// ָ����Ҫ��Ⱦ��Ŀ�껺����
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());  // ?

	// ������������
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// ���ø�ǩ��
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// ��������װ��׶εĶ��㡢����������
	mCommandList->IASetVertexBuffers(0, 1, &mPyramidGeo->VertexBufferView());
	mCommandList->IASetIndexBuffer(&mPyramidGeo->IndexBufferView());

	// ����ͼԪ����Ϊ�������б�ԭ��ΪD3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST��
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ����������
	mCommandList->SetGraphicsRootDescriptorTable(
		0,  // ��HLSL�еĴ���register(b0)��Ӧ 
		mCbvHeap->GetGPUDescriptorHandleForHeapStart());

	// ����ʵ��
	mCommandList->DrawIndexedInstanced(
		mPyramidGeo->DrawArgs["pyramid"].IndexCount,
		1, 0, 0, 0);

	// ����Դ����;ָʾ��״̬��ת�䣬�˴�����Դ����ȾĿ��״̬ת��Ϊ����״̬
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// �������ļ�¼
	ThrowIfFailed(mCommandList->Close());

	// ��������������ִ�е������б�
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// ���ֻ��������ݲ�������̨��������ǰ̨������
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// �ȴ����ƴ�֡��һϵ������ִ�����
	FlushCommandQueue();
}

void PyramidApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void PyramidApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void PyramidApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	// ������
	if ((btnState & MK_LBUTTON) != 0)
	{
		// ���������ƶ����������ת�Ƕȣ���ÿ�����ذ��˽Ƕȵ�1/4������ת
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// �����������������������������׶��ת�ĽǶ�
		mTheta += dx;
		mPhi += dy;

		// ���ƽǶ�mPhi�ķ�Χ
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	// ����Ҽ�
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// ʹ�����е�ÿ�����ذ�����ƶ������0.005����������
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// ���������������������Ŀ��ӷ�Χ�뾶
		mRadius += dx - dy;

		// ���ƿ��Ӱ뾶�ķ�Χ
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

// �����������ѣ�����ֻ�ô���CBV�������ѣ�
void PyramidApp::BuildDescriptorHeaps()
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
void PyramidApp::BuildConstantBuffers()
{
	// �����˴洢��һ��ObjectConstants���͵ĳ��������������飩
	// �û������洢�˻���n����������ĳ������ݣ����ڱ���ֻ��Ҫ����һ������׶��n = 1
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	// ��256B��������Ϊ���������
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// ����������ʼ��ַ��������Ϊ0���Ǹ������������ĵ�ַ��
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

	// ƫ�Ƶ������������е�i����������Ӧ�ĳ������ݣ�����ȡi = 0
	int pyramidCBufIndex = 0;
	cbAddress += pyramidCBufIndex * objCBByteSize;

	// cbvDesc�������ǰ󶨵�HLSL�����������ṹ��ĳ�����������Դ�Ӽ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

// ������ǩ��
void PyramidApp::BuildRootSignature()
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
void PyramidApp::BuildShadersAndInputLayout()
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

// ��������׶
void PyramidApp::BuildPyramidGeometry()
{
	// ��������׶���������
	std::array<Vertex, 5> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),  // v0
		Vertex({ XMFLOAT3(+0.0f, +0.7f, +0.0f), XMFLOAT4(Colors::Red) }),    // v1
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),  // v2
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),  // v3
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),  // v4
	};

	// ��������׶������
	std::array<std::uint16_t, 18> indices =
	{
		// ǰ����
		0, 1, 2,

		// �����
		3, 1, 4,

		// �����
		0, 4, 1,

		// �ұ���
		1, 3, 2,

		// �±���
		0, 2, 4,
		2, 3, 4
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mPyramidGeo = std::make_unique<MeshGeometry>();
	mPyramidGeo->Name = "mPyramidGeo";

	// ���������ݸ��Ƶ�CPU�ڴ���
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mPyramidGeo->VertexBufferCPU));
	CopyMemory(mPyramidGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	// ���������ݸ��Ƶ�CPU�ڴ���
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mPyramidGeo->IndexBufferCPU));
	CopyMemory(mPyramidGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	// ͨ���������ݴ�����Ӧ�Ķ��㻺������Դ
	mPyramidGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mPyramidGeo->VertexBufferUploader);

	// ͨ���������ݴ�����Ӧ��������������Դ
	mPyramidGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mPyramidGeo->IndexBufferUploader);

	mPyramidGeo->VertexByteStride = sizeof(Vertex);
	mPyramidGeo->VertexBufferByteSize = vbByteSize;
	mPyramidGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mPyramidGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mPyramidGeo->DrawArgs["pyramid"] = submesh;
}

// ������ˮ��״̬����
void PyramidApp::BuildPSO()
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
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;						   // ���ز��ö�ÿ�����صĲ�������
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;      // ���ز�������������
	psoDesc.DSVFormat = mDepthStencilFormat;                                   // ���/ģ�建�����ĸ�ʽ

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}