/*
*  上传缓冲区辅助操作类
*  UploadBuffer.h
*  作者： Frank Luna (C)
*/

#pragma once

#include "d3dUtil.h"

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
		mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);

		// 常量缓冲区的大小为256B的整数倍
		// 因为硬件只能按m*256B的偏移量和n*256B的数据长度这两种规格来查看常量数据
		//typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		//	UINT64 OffsetInBytes; // 256的整数倍
		//	UINT   SizeInBytes;   // 256的整数倍
		//} D3D12_CONSTANT_BUFFER_VIEW_DESC;

		// 如果是常量缓冲区
		if(isConstantBuffer)
			mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

		// 创建上传缓冲区
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));

		// 完成映射
		ThrowIfFailed(mUploadBuffer->Map(
			0,										   // 子资源索引
			nullptr,								   // 内存映射范围（当前表示对整个资源进行映射）
			reinterpret_cast<void**>(&mMappedData)));  // 返回待映射资源数据的目标内存块

		// 只要还会修改当前的资源，我们就无法取消映射
		// 但在资源被GPU使用期间，切忌向该资源进行写操作（需借助同步技术）
	}

	// 禁止使用如下函数
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

	// 在该类的析构函数中完成取消映射和释放映射内存
	~UploadBuffer()
	{
		// 在释放映射内存之前对其先进行取消映射操作
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		// 释放映射内存
		mMappedData = nullptr;
	}

	// 获取上传缓冲区指针
	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}

	// 将数据从CPU端控制的内存复制到常量缓冲区
	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};