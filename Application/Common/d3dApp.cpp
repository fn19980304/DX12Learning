/*
*  Direct3DӦ�ó�����ʵ��
*  d3dApp.cpp
*  ���ߣ� Frank Luna (C)
*/

#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// ���ڹ��̣��ڴ�����Ϣʱ���Զ�����
	// ʵ�ʵĴ��ڹ��̴�����ΪMsgProc����

	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
	// ���һ��D3DӦ�ó���ʵ��

	return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance) :mhAppInst(hInstance)
{
	// �򵥵ؽ����ݳ�Ա��ʼ��ΪĬ��ֵ

	// mhAppInst = hInstance
	// ֻ������һ��D3DApp
	assert(mApp == nullptr);
	mApp = this;
}

D3DApp::~D3DApp()
{
	// �ͷ�D3DApp�����õ�COM�ӿڶ���ˢ���������

	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

HINSTANCE D3DApp::AppInst()const
{
	// ����Ӧ�ó���ʵ�����

	return mhAppInst;
}

HWND D3DApp::MainWnd()const
{
	// ���������ھ��

	return mhMainWnd;
}

float D3DApp::AspectRatio()const
{
	// �ݺ�ȣ���ʵ�Ǻ�̨���������/�߶ȣ�

	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState()const
{
	// ��ȡ4X MSAA�������

	return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
	// ���������4X MSAA

	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;

		// ���ý������ͻ�����
		CreateSwapChain();
		OnResize();
	}
}

int D3DApp::Run()
{
	// ���д���
	// û�д�����Ϣ����ʱ�ͻᴦ�����ǵ���Ϸ�߼�����

	// ��Ϣѭ��
	// ������Ϣ�ṹ��
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT) {
		// ����д�����Ϣ�ͽ��д���
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);  // ���̰������������Ϣת��Ϊ�ַ���Ϣ
			DispatchMessage(&msg);   // ��Ϣ����
		}
		// �����ִ�ж�������Ϸ������߼�
		else {
			mTimer.Tick();

			if (!mAppPaused) {
				CalculateFrameStats();
				Update(mTimer);
				Draw(mTimer);
			}
			else {
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool D3DApp::Initialize()
{
	// ��ʼ������

	// ��ʼ������
	if (!InitMainWindow())
		return false;

	// ��ʼ��D3D
	if (!InitDirect3D())
		return false;

	// ������С
	OnResize();

	return true;
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// ������������

	// RTV�Ѵ���
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;  // RTV��Ե���ǰ����̨������
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// DSV�Ѵ���
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;						// ֻ����һ��DSV�ѣ�DSV��Ժ�̨��������
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::OnResize()
{
	// ����Ŀ����Ⱦ��ͼ�Լ����ڹ����������������С

	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// �ڸı��κ���Դ��ǰˢ��ˢ��������У������������б�
	FlushCommandQueue();
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// �ͷ�֮ǰ����Դ
	for (int i = 0; i < SwapChainBufferCount; ++i) {
		mSwapChainBuffer[i].Reset();
	}
	mDepthStencilBuffer.Reset();

	// �޸Ľ�����
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	// Ϊ�������е�ÿһ������������һ��RTV
	// ���ȴ���һ��������¼��ǰ����RTV�ڶ�������λ�õľ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		// ��ô��ڽ������еĵ�i����������Դ
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));

		// Ϊ����Դ����һ��RTV
		md3dDevice->CreateRenderTargetView(
			mSwapChainBuffer[i].Get(),  // ָ��������ȾĿ�����Դ����̨�������� 
			nullptr,					// ��Դ��Ԫ�ص����ݸ�ʽ 
			rtvHeapHandle);

		// ƫ�Ƶ��������ѵ���һ��������
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// �������ģ�建����������ͼ
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;				// ��Դά�ȣ�ʹ��2D����
	depthStencilDesc.Alignment = 0;													// ����
	depthStencilDesc.Width = mClientWidth;											// ������Ϊ��λ��������
	depthStencilDesc.Height = mClientHeight;										// ������Ϊ��λ������߶�
	depthStencilDesc.DepthOrArraySize = 1;											// ������Ϊ��λ��������ȣ�2D��������Ĵ�С��
	depthStencilDesc.MipLevels = 1;													// mipmap�㼶
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;							// ���ظ�ʽ
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;						// ���ز�������
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;  // ���ز�����������
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                         // ������
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;														// �����Դ���Ż�ֵ
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),			  // �ѵ�����
		D3D12_HEAP_FLAG_NONE,										  // �ѵĶ����־
		&depthStencilDesc,											  // ������Դ
		D3D12_RESOURCE_STATE_COMMON,								  // ������Դ״̬Ϊ��ʼ״̬
		&optClear,													  // �����Դ���Ż�ֵ
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// ���ø���Դ��ʽ��Ϊ������Դ�ĵ�0mip�㴴��������
	md3dDevice->CreateDepthStencilView(
		mDepthStencilBuffer.Get(),  // ָ�����ģ�建������Դ
		nullptr,					// ��Դ��Ԫ�ص����ݸ�ʽ������Ϊnullptr���ٷ�������ʹ��dsvDesc��
		DepthStencilView());

	// ����Դ�ӳ�ʼ״̬ת��Ϊ��Ȼ�����
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// ִ�е�����С����
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// ������С����ǰ��CPU�ȴ�
	FlushCommandQueue();

	// �����ӿ�
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	// ��֮��ʹ�øÿ�ܱ��ʱ���Ե���
	// RSSetViewports��RSSetScissorRects���������ӿںͲü�����
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// ���ڹ��̺���

	switch (msg)
	{
		// ��һ�����򼤻activate����δ���deactivate��ʱ��ᷢ��WM_ACTIVATE��Ϣ
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			// ��ͣ��Ϸ��deactivated����ֹͣ��ʱ
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			// ������Ϸ��actived������ʼ��ʱ
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

		// �����Ǹı䴰�ڴ�С��ʱ��ͻ����WM_SIZE��Ϣ
	case WM_SIZE:
		// ��¼�µĴ��ڴ�С
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// �Ƿ����С���ָ�
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// �Ƿ����󻯻ָ�
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// ���û��϶�������ʱ������������WM_SIZE����ʱ���ǲ�Ӧ����֮
					// ����������������С�����ԣ���ʱӦʲô��������������ͣ���򣩣�
					// ֱ���û�����������������ִ��OnResize����
				}
				else
				{
					// ��SetWindowPos��mSwapChain->SetFullscreenState������API����
					OnResize();
				}
			}
		}
		return 0;

		// ���û�ץȡ������ʱ�ᷢ��WM_EXITSIZEMOVE��Ϣ
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		// ���û��ͷŵ�����ʱ�ᷢ��WM_EXITSIZEMOVE��Ϣ
		// ��ʱ�����µĴ��ڴ�С������ض��󣨻���������ͼ�ȣ�
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		// �����ڱ�����ʱ����WM_DESTROY��Ϣ
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// ��ĳһ�˵����ڼ���״̬���û����µļȲ������Ǽ��ֲ��Ǽ��ټ�ʱ
		// ����WM_MENUCHAR��Ϣ
	case WM_MENUCHAR:
		// ��������ϼ�alt-enterʱ���ᷢ��beep������
		return MAKELRESULT(0, MNC_CLOSE);

		// �������Ϣ�Է����ڱ�ù�С
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

		// �����·�ʽ����������йص���Ϣ
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	// ��ʼ��Windows����

	// Step1:��д���ڳ�ʼ�������ṹ��(WNDCLASS)
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;				         // ��������߸ı��ػ洰��
	wc.lpfnWndProc = MainWndProc;							 // �󶨴��ڹ���
	wc.cbClsExtra = 0;										 // ���������ֶ�Ϊ��ǰӦ�÷��������ڴ�ռ�
	wc.cbWndExtra = 0;										 // ������ʱ=0
	wc.hInstance = mhAppInst;								 // Ӧ�ó���ʵ��
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);				 // Ӧ�ó���ͼ�꣺Ĭ��
	wc.hCursor = LoadCursor(0, IDC_ARROW);					 // ���ָ����ʽ��Ĭ��
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  // ������ɫ����ɫ
	wc.lpszMenuName = 0;									 // �����ò˵���
	wc.lpszClassName = L"MainWnd";							 // ������

	if (!RegisterClass(&wc)) {
		// ע�ᴰ��
		// ���ʧ�ܱ���
		MessageBox(0, L"RegisterClass FAILED", 0, 0);
		return false;
	}

	// Step2:����������û�����ߴ���㴰�ھ��γߴ�
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	// Step3:ע��ɹ�������ߴ�󴴽�����
	//       �÷�������һ��HWND���͵Ĵ��ھ��������ʧ��ʱ����0
	mhMainWnd = CreateWindow(
		L"MainWnd",				  // ʹ��ǰ��ע���WNDCLASSʵ����������wc.lpszClassNameһ��
		mMainWndCaption.c_str(),  // ���ڱ���
		WS_OVERLAPPEDWINDOW,	  // ������ʽʾ��
		CW_USEDEFAULT,			  // x����
		CW_USEDEFAULT,			  // y����
		width,				      // ���ڿ�
		height,				      // ���ڸ�
		0,						  // ������
		0,						  // �˵�
		mhAppInst,				  // Ӧ�ó���ʵ��
		0
	);
	if (mhMainWnd == 0) {
		// ����ʧ�ܱ���
		MessageBox(0, L"CreateWindow FAILED", 0, 0);
		return false;
	}

	// Step4:չʾ���ڲ�����
	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
	// ��ʼ��Direct3D
#if defined(DEBUG) || defined(_DEBUG) 
	// ����D3D12�ĵ��Բ�
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	// Step1:��D3D12CreateDevice��������ID3D12Device�ӿ�ʵ��

	// ���ȴ���DXGI�����ӿ�
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// ��DXGI�����ӿ�֮�ϴ���D3D12�豸
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,				   // Ĭ��������
		D3D_FEATURE_LEVEL_11_0,    // ֧��D3D11����
		IID_PPV_ARGS(&md3dDevice)  // ����ID3D12Device��COMID
	);

	// ������WARP�豸
	if (FAILED(hardwareResult)) {
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}


	// Step2:����һ��ID3D12Fence���󣬲���ѯ�������Ĵ�С

	// ����Χ��
	ThrowIfFailed(md3dDevice->CreateFence(
		0,  // ���Χ���㣬��ֵΪ0
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	// ��ѯ��������С�������������������ҵ���Ӧ������
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	// Step3:����û��豸��4X MSAA���������֧�����

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;       // ����4X MSAA
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;  // ��������
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));  // �ڶ������������������������ѯ����дNumQualityLevels

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");  // ��֧��MSAAӦ����>0������ͱ���

#ifdef _DEBUG
	LogAdapters();
#endif

	// Step4:���δ���������С������б���������������б�

	CreateCommandObjects();

	// Step5:����������������

	CreateSwapChain();

	// Step6:����Ӧ���������������

	CreateRtvAndDsvDescriptorHeaps();

	// Step7:������̨��������С����Ϊ��������ȾĿ����ͼ
	// Step8:�����ӿںͲü�����

	// OnResize();

	return true;
}

void D3DApp::CreateCommandObjects()
{
	// ����������к������б�

	// �������
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  // �洢��������GPU��ֱ��ִ�е�
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// ����������
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	// �����б�
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(),  // �������������
		nullptr,                    // ��ʼ����ˮ��״̬����
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// �ڵ�һ�����������б�ǰҪ���ã����ڵ������÷���ǰҪ�ȹر�
	mCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// ����������������

	// ����Ӧ���ͷ�֮ǰ�Ľ����������ؽ�
	mSwapChain.Reset();

	// ��дDXGI_SWAP_CHAIN_DESC�ṹ��
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;								        // �������ֱ��ʿ��
	sd.BufferDesc.Height = mClientHeight;							        // �������ֱ��ʸ߶�
	sd.BufferDesc.RefreshRate.Numerator = 60;						        // ˢ���ʷ�ĸ
	sd.BufferDesc.RefreshRate.Denominator = 1;						        // ˢ���ʷ���
	sd.BufferDesc.Format = mBackBufferFormat;							    // ��������ʾ��ʽ
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;  // ���� vs. ����ɨ�裨δָ���� 
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;					// ͼ��������죨δָ����
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;								// ���ز�������
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;		// ���ز�����������
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;                       // ����̨��������Ϊ��ȾĿ��
	sd.BufferCount = SwapChainBufferCount;                                  // �������еĻ�����������˫���壩
	sd.OutputWindow = mhMainWnd;											// ��Ⱦ����
	sd.Windowed = true;														// ����ģʽ
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;						// �л�ȫ������Ӧ

	// ͨ���������ˢ�½�����
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	// ǿ��CPU�ȴ�GPU���Խ��ͬ�����⣬��CPU���ܲ���ͬ������ǰ��Ҫ���ø÷���ˢ���������

	// ����Χ��ֵ���������������ǵ���Χ����
	mCurrentFence++;

	// CPU����GPU��������������һ������������Χ���������
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// ��CPU�˵ȴ�GPU��ֱ������ִ�������Χ����֮ǰ����������
	if (mFence->GetCompletedValue() < mCurrentFence) {
		// ��������ʵ��CPU�ȴ����¼�
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		// ��GPU���е�ǰ��Χ������ִ�е�Signal()ָ��޸���Χ��ֵ�����򼤷�Ԥ���¼�
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// �ȴ�GPU����Χ�������¼������¼�����δ������is not signaled��CPUһֱ���ڵȴ�״̬
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	// ���ؽ������е�ǰ��̨��������ID3D12Resource

	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	// ���ص�ǰ��̨��������RTV���

	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),  // ���е��׸����
		mCurrBackBuffer,                                 // ƫ������̨���������������������
		mRtvDescriptorSize);                             // ��������ռ�ֽڵĴ�С  
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	// ���������ģ��ģ�建������DSV���

	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
	// ����ÿ���ƽ��֡���Լ�ÿ֡��ƽ����Ⱦʱ��
	// ��Щͳ����Ϣ���ᱻ���ӵ����ڵı�������

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// ��1��Ϊͳ������������ƽ��֡���Լ�ÿ֡��ƽ����Ⱦʱ��
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f) {
		float fps = (float)frameCnt;  // fps = frameCnt / 1
		float mspf = 1000.0f / fps;


		// ��ʾ�ı����ݵ�����
		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Ϊ������һ��ƽ��ֵ����
		frameCnt = 0;
		timeElapsed += 1.0f;
	}

}

void D3DApp::LogAdapters()
{
	// ö��ϵͳ��������ʾ����������������Կ���

	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	// ö����ĳ��������������������ʾ���������һ���Կ��������������ʾ����

	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	// ��ȡĳ����ʾ����Ծ����ʽ��֧�ֵ�ȫ����ʾģʽ

	UINT count = 0;
	UINT flags = 0;

	// ��nullptr��Ϊ�������ô˺�������ȡ������������ʾģʽ�ĸ���
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}