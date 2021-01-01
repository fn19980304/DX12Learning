/*
*  第6章习题4
*  Exercise7.cpp
*
*  控制：
*		 按下鼠标左键拖动以旋转
*		 按下鼠标右键拖动以缩放
*
*  作者： fn19980304
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

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 6.5f;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// 针对调试版本开启运行时内存检测
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		BoxApp theApp(hInstance);
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

	BuildDescriptorHeaps();		   // 创建描述符堆
	BuildConstantBuffers();		   // 创建常量缓冲区
	BuildRootSignature();          // 构建根签名
	BuildShadersAndInputLayout();  // 编译着色器程序、构建输入布局描述
	BuildBoxGeometry();            // 构建立方体
	BuildPSO();                    // 构建流水线状态对象

	// 执行初始化命令
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 等待初始化完成
	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// 若用户调整了窗口尺寸，则更新纵横比并重新计算投影矩阵
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25 * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
	// 将球坐标转换为笛卡尔坐标
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// 构建矩阵
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX worldBox = XMLoadFloat4x4(&mWorld);
	XMMATRIX worldPyramid = XMMatrixTranslation(-2.0f, 0.0f, 0.0f);

	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX boxWorldViewProj = worldBox * view * proj;
	XMMATRIX pyramidWorldViewProj = worldPyramid * view * proj;

	// 更新立方体常量缓冲区
	ObjectConstants boxConstants;
	XMStoreFloat4x4(&boxConstants.WorldViewProj, XMMatrixTranspose(boxWorldViewProj));
	mObjectCB->CopyData(0, boxConstants);

	// 更新四棱锥常量缓冲区
	ObjectConstants pyramidConstants;
	XMStoreFloat4x4(&pyramidConstants.WorldViewProj, XMMatrixTranspose(pyramidWorldViewProj));
	mObjectCB->CopyData(1, pyramidConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
	// 复用记录命令所用的内存
	// 只有当GPU中的命令列表执行完毕后，我们才可以对其进行重置
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// 通过函数ExecuteCommandList将命令列表加入命令队列后，便可对它进行重置
	// 复用命令列表即复用其相应的内存
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	// 设置视口和裁剪矩形
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 按照资源的用途指示其状态的转变，此处将资源从呈现状态转换为渲染目标状态
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 清楚后台缓冲区和深度缓冲区
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 指定将要渲染的目标缓冲区
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView()); 

	// 设置描述符堆
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// 设置根签名
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// 设置输入装配阶段的顶点、索引缓冲区
	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());

	// 设置图元拓扑为三角形列表（原文为D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST）
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 绑定立方体的描述符表
	mCommandList->SetGraphicsRootDescriptorTable(
		0,  // 与HLSL中的代码register(b0)对应 
		mCbvHeap->GetGPUDescriptorHandleForHeapStart());

	// 绘制实例立方体
	mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs["box"].IndexCount,
		1, 0, 0, 0);

	// 绑定四棱锥的描述符表，此时会更新寄存器b0中的数据，以此以新的世界矩阵绘制四棱锥使其与矩阵不相交
	CD3DX12_GPU_DESCRIPTOR_HANDLE pyramidCbv(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	pyramidCbv.Offset(1, mCbvSrvUavDescriptorSize);  // 获得四棱锥常量缓冲区的描述符
	mCommandList->SetGraphicsRootDescriptorTable(
		0,  // 与HLSL中的代码register(b0)对应 
		pyramidCbv);

	// 绘制实例四棱锥
	mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs["pyramid"].IndexCount,
		1, mBoxGeo->DrawArgs["pyramid"].StartIndexLocation, mBoxGeo->DrawArgs["pyramid"].BaseVertexLocation, 0);

	// 按资源的用途指示其状态的转变，此处将资源从渲染目标状态转换为呈现状态
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// 完成命令的记录
	ThrowIfFailed(mCommandList->Close());

	// 向命令队列添加欲执行的命令列表
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 呈现缓冲区内容并交换后台缓冲区与前台缓冲区
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 等待绘制此帧的一系列命令执行完毕
	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	// 鼠标左键
	if ((btnState & MK_LBUTTON) != 0)
	{
		// 根据鼠标的移动距离计算旋转角度，令每个像素按此角度的1/4进行旋转
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// 根据鼠标的输入来更新摄像机绕立方体旋转的角度
		mTheta += dx;
		mPhi += dy;

		// 限制角度mPhi的范围
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	// 鼠标右键
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// 使场景中的每个像素按鼠标移动距离的0.005倍进行缩放
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// 根据鼠标的输入更新摄像机的可视范围半径
		mRadius += dx - dy;

		// 限制可视半径的范围
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

// 创建描述符堆（本章只用创建CBV描述符堆）
void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 2;
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
	// 该缓冲区存储了绘制n个物体所需的常量数据，由于本题需要绘制一个立方体和一个四棱锥，n = 2
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 2, true);

	// 以256B的整数倍为其填充数据
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// 缓冲区的起始地址（即索引为0的那个常量缓冲区的地址）
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

	// 创建立方体的常量缓冲区描述符
	int boxCBufIndex = 0;

	D3D12_CONSTANT_BUFFER_VIEW_DESC boxCbvDesc;
	boxCbvDesc.BufferLocation = cbAddress + boxCBufIndex * objCBByteSize;
	boxCbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(
		&boxCbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());

	// 创建四棱锥的常量缓冲区描述符
	int pyramidCBufIndex = 1;

	D3D12_CONSTANT_BUFFER_VIEW_DESC pyramidCbvDesc;
	pyramidCbvDesc.BufferLocation = cbAddress + pyramidCBufIndex * objCBByteSize;
	pyramidCbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());  // 见第7章
	handle.Offset(1, mCbvSrvUavDescriptorSize);
	md3dDevice->CreateConstantBufferView(
		&pyramidCbvDesc,
		handle);
}

// 创建根签名
void BoxApp::BuildRootSignature()
{
	// 着色器程序一般需要以资源作为输入（常量缓冲区、纹理、采样器等）
	// 根签名则定义了着色器程序所需要的具体资源
	// 如果把着色器程序看作一个函数，而将输入的资源当作向函数传递的参数数据
	// 那么便可类似地认为根签名定义的是函数签名

	// 根参数可以是描述符表、根描述符或根常量
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// 创建由单个CBV所组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  // 描述符表的类型 
		1,                                // 表中描述符的数量
		0);                               // 将这段描述符区域绑定至基址着色器寄存器

	// 以描述符表初始化根参数
	slotRootParameter[0].InitAsDescriptorTable(
		1,           // 描述符区域数量
		&cbvTable);  // 指向描述符区域数组的指针 

	// 根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 先将根签名的描述布局进行序列化处理
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	// 创建仅含一个槽位（该槽位指向一个仅由单个常量缓冲区组成的描述符区域，即cbvTable）的根签名
	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

// 编译着色器字节码和输入布局描述
void BoxApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	// 顶点着色器、像素着色器
	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	// 构成输入布局描述的数组
	// 语义, 附加到语义上的索引, 元素格式（数据类型）, 输入槽, 偏移量, 高级技术备选项, 高级技术备选项
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

// 构建立方体
void BoxApp::BuildBoxGeometry()
{
	// 定义立方体的8个顶点及四棱锥的5个顶点
	std::array<Vertex, 13> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),  
		Vertex({ XMFLOAT3(+0.0f, +0.7f, +0.0f), XMFLOAT4(Colors::Red) }),    
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),  
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),  
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),  
	};

	// 定义立方体和四棱锥的索引
	std::array<std::uint16_t, 54> indices =
	{
		// 立方体
		// 前表面
		0, 1, 2,
		0, 2, 3,

		// 后表面
		4, 6, 5,
		4, 7, 6,

		// 左表面
		4, 5, 1,
		4, 1, 0,

		// 右表面
		3, 2, 6,
		3, 6, 7,

		// 上表面
		1 ,5, 6,
		1, 6, 2,

		// 下表面
		4, 0, 3,
		4, 3, 7,

		// 四棱锥
		// 前表面
		0, 1, 2,

		// 后表面
		3, 1, 4,

		// 左表面
		0, 4, 1,

		// 右表面
		1, 3, 2,

		// 下表面
		0, 2, 4,
		2, 3, 4
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	// 将顶点数据复制到CPU内存中
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	// 将索引数据复制到CPU内存中
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	// 通过顶点数据创建相应的顶点缓冲区资源
	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	// 通过索引数据创建相应的索引缓冲区资源
	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	// 立方体
	SubmeshGeometry submeshBox;
	submeshBox.IndexCount = 36;
	submeshBox.StartIndexLocation = 0;
	submeshBox.BaseVertexLocation = 0;
	mBoxGeo->DrawArgs["box"] = submeshBox;

	// 四棱锥
	SubmeshGeometry submeshPyramid;
	submeshPyramid.IndexCount = 18;
	submeshPyramid.StartIndexLocation = 36;
	submeshPyramid.BaseVertexLocation = 8;
	mBoxGeo->DrawArgs["pyramid"] = submeshPyramid;
}

// 构建流水线状态对象
void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };  // 输入布局描述
	psoDesc.pRootSignature = mRootSignature.Get();                             // 根签名指针
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};                                                                         // 顶点着色器
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};											                               // 像素着色器

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);          // 光栅化状态
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);					   // 混合状态
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);     // 深度/模板状态
	psoDesc.SampleMask = UINT_MAX;											   // 对所有点进行采样
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;    // 图元的拓扑类型
	psoDesc.NumRenderTargets = 1;											   // 同时所用的渲染目标数量
	psoDesc.RTVFormats[0] = mBackBufferFormat;								   // 渲染目标格式
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;						   // 多重采用对每个像素的采样数量
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;      // 多重采样的质量级别
	psoDesc.DSVFormat = mDepthStencilFormat;                                   // 深度/模板缓冲区的格式

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}