#pragma once

#define NOMINMAX 1
#include <windows.h>
#include <comdef.h>
#include <string>
#include <format>
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

#include "inc/magic_enum.hpp"

inline std::wstring to_wstring(const std::string& s)
{
    std::wstring wide_string;

    // NOTE: be sure to specify the correct codepage that the
    // str::string data is actually encoded in...
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), static_cast<int>(s.size()), NULL, 0);
    if (len > 0) {
        wide_string.resize(len);
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), static_cast<int>(s.size()), &wide_string[0], len);
    }

    return wide_string;
}

inline std::string to_string(const std::wstring& s)
{
    std::string string;

    // NOTE: be sure to specify the correct codepage that the
    // str::string data is actually encoded in...
    int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.c_str(), static_cast<int>(s.size()), nullptr, 0, NULL, NULL);
    if (len > 0) {
        string.resize(len);
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.c_str(), static_cast<int>(s.size()), &string[0], len, NULL, NULL);
    }

    return string;
}

class DxVkException
{
private:
    enum class ResultType : int
    {
        Dx,
        Vk,
        SpvReflect
    };

    DxVkException(const std::wstring& functionName, const std::wstring& filename, int lineNumber)
        : FunctionName(functionName), Filename(filename), LineNumber(lineNumber)
    {
    }

public:
    DxVkException() = default;
    DxVkException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
        : DxErrorCode(hr), ErrorType(ResultType::Dx)
    {
        DxVkException(functionName, filename, lineNumber);
    }

    DxVkException(VkResult vr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
        : VkErrorCode(vr), ErrorType(ResultType::Vk)
    {
        DxVkException(functionName, filename, lineNumber);
    }

    DxVkException(SpvReflectResult sr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
        : SpvReflectErrorCode(sr), ErrorType(ResultType::SpvReflect)
    {
        DxVkException(functionName, filename, lineNumber);
    }

    std::wstring ToString()const
    {
        std::wstring msg;
        switch (ErrorType)
        {
        case ResultType::Dx:
        {
            _com_error err(DxErrorCode);
            msg = err.ErrorMessage();
            break;
        }
        case ResultType::Vk:
        {
            msg = to_wstring(std::string(magic_enum::enum_name(VkErrorCode)));
            break;
        }
        case ResultType::SpvReflect:
        {
            msg = to_wstring(std::string(magic_enum::enum_name(SpvReflectErrorCode)));
            break;
        }
        default:
            msg = L"[Error Result Type]";
        }

        return std::format(L"{} failed in {}; line {}; error: {}", FunctionName, Filename, LineNumber, msg);

    }

    union
    {
        HRESULT DxErrorCode;
        VkResult VkErrorCode;
        SpvReflectResult SpvReflectErrorCode;
    };
    
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;

    ResultType ErrorType;
};

//inline std::wstring AnsiToWString(const std::string& str)
//{
//    WCHAR buffer[512];
//    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
//    return std::wstring(buffer);
//}


template <class T>
bool IsSuccess(T res)
{
    return true;
}

template<HRESULT>
bool IsSuccess(HRESULT hr)
{
    return SUCCEEDED(hr);
}

template<VkResult>
bool IsSuccess(VkResult vr)
{
    return vr == VK_SUCCESS;
}

template<SpvReflectResult>
bool IsSuccess(SpvReflectResult sr)
{
    return sr == SPV_REFLECT_RESULT_SUCCESS;
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    auto res__ = (x);                                               \
    std::wstring wfn = to_wstring(__FILE__);                       \
    if(!IsSuccess(res__)) { throw DxVkException(res__, L## #x, wfn, __LINE__); } \
}
#endif
