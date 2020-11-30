/*
*  初始化Direct3D演示程序
*  InitDirect3DApp.cpp
*  作者： Frank Luna (C)
*/

#include "../../Common/d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp :public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// 为调试版本开启运行时的内存检测，方便监督内存泄露的情况
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try {
		InitDirect3DApp theApp(hInstance);
		if (!theApp.Initialize()) 
			return 0;

		return theApp.Run();
	}
	catch (DxException& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance) :D3DApp(hInstance)
{

}

InitDirect3DApp::~InitDirect3DApp()
{

}

bool InitDirect3DApp::Initialize()
{
	// 在之后自己实现的初始化派生方法中，通过调用D3DApp类中的初始化方法
	// 访问D3DApp类中的初始化成员

	if (!D3DApp::Initialize())
		return false;

	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt)
{

}

void InitDirect3DApp::Draw(const GameTimer& gt)
{
	// 重复使用记录命令的相关内存
	// 只有当与GPU关联的命令列表执行完成时，我们才能将其重置
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// 在通过执行ExecuteCommandList方法将某个命令列表加入命令队列后
	// 我们便可重置该命令列表，以此来复用命令列表及其内存
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 对资源的状态进行转换，将资源从呈现状态转换为渲染目标状态
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, 
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 设置视口和裁剪矩形。它们需要随着命令列表的重置而重置
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 清除后台缓冲区和深度缓冲区
	mCommandList->ClearRenderTargetView(
		CurrentBackBufferView(),							// 待清除的资源RTV
		Colors::LightSteelBlue,								// 清楚后的渲染目标填充颜色
		0,													// pRects数组中的元素数量 
		nullptr);											// pRects（指定渲染目标将要被清除的矩形区域）
	mCommandList->ClearDepthStencilView(
		DepthStencilView(),									// 待清除的资源DSV
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,  // 同时清除深度和模板缓冲区
		1.0f,                                               // 以此值来清除深度缓冲区
		0,                                                  // 以此值来清除模板缓冲区
		0,                                                  // pRects数组中的元素数量
		nullptr);								            // pRects（指定资源视图将要被清除的矩形区域）

	// 指定将要渲染的缓冲区
	mCommandList->OMSetRenderTargets(
		1,                         // 待绑定RTV数量
		&CurrentBackBufferView(),  // 待绑定RTV数组
		true,                      // RTV数组中的对象是否在描述符堆中连续存放
		&DepthStencilView());      // 待绑定DSV

	// 再对资源状态进行转换，将资源从渲染目标状态呈现状态
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	// 完成命令的记录
	ThrowIfFailed(mCommandList->Close());

	// 将待执行的命令列表加入命令列表
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换后台缓冲区和前台缓冲区
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 等待此帧的命令执行完毕
	FlushCommandQueue();
}
