#include <AK/Try.h>
#include <fcntl.h>
#include <LibCore/AnonymousBuffer.h>
#include <LibCore/System.h>
#include <LibIPC/File.h>

namespace Core {

ErrorOr<HANDLE> CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName)
{
	HANDLE hFileMappingObject = ::CreateFileMappingA(hFile, lpAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
	if (hFileMappingObject == INVALID_HANDLE_VALUE || hFileMappingObject == nullptr)
		return Error::from_string_view("CreateFileMappingW failed"sv);
	return hFileMappingObject;
}

ErrorOr<LPVOID> MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap)
{
	LPVOID lpBaseAddress = ::MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
	if (lpBaseAddress == nullptr)
		return Error::from_string_view("MapViewOfFile failed"sv);
	return lpBaseAddress;
}

ErrorOr<AnonymousBuffer> AnonymousBuffer::create_with_size(size_t size)
{
	TCHAR name[] = TEXT("Local\\AnonymousBuffer-XXXXXX");


    auto handle = TRY(CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        size,
        name));

	return create_from_anon_handle(handle, size);
}

ErrorOr<NonnullRefPtr<AnonymousBufferImpl>> AnonymousBufferImpl::create(HANDLE handle, size_t size)
{
    auto* data = static_cast<void*>(::MapViewOfFile(
        handle,
        FILE_MAP_WRITE,
        0,
        0,
        size));
    if (!data)
        return Error::from_string_view("MapViewOfFile failed"sv);
    return AK::adopt_nonnull_ref_or_enomem(new (nothrow) AnonymousBufferImpl(handle, size, data));
}

AnonymousBufferImpl::~AnonymousBufferImpl()
{
    if (m_file_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_file_handle);
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
    : m_file_handle(handle)
    , m_size(size)
    , m_data(data)
{
}

}
