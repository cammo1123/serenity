/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2023, Cameron Youell <cameronyouell@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/AnonymousBuffer.h>
#include <LibCore/System.h>

namespace Core {

ErrorOr<HANDLE> CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName)
{
    HANDLE hFileMappingObject = ::CreateFileMappingA(hFile, lpAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
    if (hFileMappingObject == INVALID_HANDLE_VALUE || hFileMappingObject == nullptr){
        return Error::from_windows_error(GetLastError());
    }
    return hFileMappingObject;
}

ErrorOr<LPVOID> MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap)
{
    LPVOID lpBaseAddress = ::MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
    if (lpBaseAddress == nullptr) {
        CloseHandle(hFileMappingObject);
        return Error::from_windows_error(GetLastError());
    }
    return lpBaseAddress;
}

ErrorOr<AnonymousBuffer> AnonymousBuffer::create_with_size(size_t size)
{
    auto handle = TRY(CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, nullptr));
    return create_from_anon_handle(handle, size);
}

ErrorOr<NonnullRefPtr<AnonymousBufferImpl>> AnonymousBufferImpl::create(HANDLE fileHandle, size_t size)
{
    void* data = TRY(MapViewOfFile(fileHandle, FILE_MAP_ALL_ACCESS, 0, 0, size));
    return AK::adopt_nonnull_ref_or_enomem(new (nothrow) AnonymousBufferImpl(fileHandle, size, data));
}

AnonymousBufferImpl::~AnonymousBufferImpl()
{
    if (m_handle != INVALID_HANDLE_VALUE) {
        auto rc = CloseHandle(m_handle);
        VERIFY(rc != 0);
    }
    UnmapViewOfFile(m_data);
}

ErrorOr<AnonymousBuffer> AnonymousBuffer::create_from_anon_handle(HANDLE handle, size_t size)
{
    auto impl = TRY(AnonymousBufferImpl::create(handle, size));
    return AnonymousBuffer(move(impl));
}

AnonymousBuffer::AnonymousBuffer(NonnullRefPtr<AnonymousBufferImpl> impl)
    : m_impl(move(impl))
{
}

AnonymousBufferImpl::AnonymousBufferImpl(HANDLE handle, size_t size, void* data)
    : m_handle(handle)
    , m_size(size)
    , m_data(data)
{
}

}
