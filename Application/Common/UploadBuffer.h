/*
*  �ϴ�����������������
*  UploadBuffer.h
*  ���ߣ� Frank Luna (C)
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

		// �����������Ĵ�СΪ256B��������
		// ��ΪӲ��ֻ�ܰ�m*256B��ƫ������n*256B�����ݳ��������ֹ�����鿴��������
		//typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		//	UINT64 OffsetInBytes; // 256��������
		//	UINT   SizeInBytes;   // 256��������
		//} D3D12_CONSTANT_BUFFER_VIEW_DESC;

		// ����ǳ���������
		if(isConstantBuffer)
			mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

		// �����ϴ�������
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));

		// ���ӳ��
		ThrowIfFailed(mUploadBuffer->Map(
			0,										   // ����Դ����
			nullptr,								   // �ڴ�ӳ�䷶Χ����ǰ��ʾ��������Դ����ӳ�䣩
			reinterpret_cast<void**>(&mMappedData)));  // ���ش�ӳ����Դ���ݵ�Ŀ���ڴ��

		// ֻҪ�����޸ĵ�ǰ����Դ�����Ǿ��޷�ȡ��ӳ��
		// ������Դ��GPUʹ���ڼ䣬�м������Դ����д�����������ͬ��������
	}

	// ��ֹʹ�����º���
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

	// �ڸ�����������������ȡ��ӳ����ͷ�ӳ���ڴ�
	~UploadBuffer()
	{
		// ���ͷ�ӳ���ڴ�֮ǰ�����Ƚ���ȡ��ӳ�����
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		// �ͷ�ӳ���ڴ�
		mMappedData = nullptr;
	}

	// ��ȡ�ϴ�������ָ��
	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}

	// �����ݴ�CPU�˿��Ƶ��ڴ渴�Ƶ�����������
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