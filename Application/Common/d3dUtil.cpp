/*
*  实用代码工具类实现
*  d3dUtil.cpp
*/

#include "d3dUtil.h"
#include <comdef.h>

using Microsoft::WRL::ComPtr;

// DxException类构造函数定义，利用初始化列表来初始化字段
DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

// DxException类成员函数ToString的定义
std::wstring DxException::ToString()const
{
    // 获得错误代码的字符串描述
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}