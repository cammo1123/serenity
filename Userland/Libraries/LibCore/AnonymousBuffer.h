/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Noncopyable.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <LibIPC/Forward.h>
#if defined(AK_OS_WINDOWS)
#    include <io.h>
#    include <windows.h>
#endif

namespace Core {

class AnonymousBufferImpl final : public RefCounted<AnonymousBufferImpl> {
public:
#if !defined(AK_OS_WINDOWS)
    static ErrorOr<NonnullRefPtr<AnonymousBufferImpl>> create(int fd, size_t);
    int fd() const { return m_fd; }
#else
    static ErrorOr<NonnullRefPtr<AnonymousBufferImpl>> create(HANDLE file_handle, size_t);
    HANDLE file_handle() const { return m_file_handle; }
#endif
    ~AnonymousBufferImpl();
    size_t size() const { return m_size; }
    void* data() { return m_data; }
    void const* data() const { return m_data; }

private:
#if !defined(AK_OS_WINDOWS)
    AnonymousBufferImpl(int fd, size_t, void*);

    int m_fd { -1 };
#else
    AnonymousBufferImpl(HANDLE file_handle, size_t, void*);

    HANDLE m_file_handle { INVALID_HANDLE_VALUE };
#endif
    size_t m_size { 0 };
    void* m_data { nullptr };
};

class AnonymousBuffer {
public:
    static ErrorOr<AnonymousBuffer> create_with_size(size_t);
    static ErrorOr<AnonymousBuffer> create_from_anon_fd(int fd, size_t);
#if !defined(AK_OS_WINDOWS)
    int fd() const { return m_impl ? m_impl->fd() : -1; }
#else
    static ErrorOr<AnonymousBuffer> create_from_anon_handle(HANDLE file_handle, size_t);
    HANDLE file_handle() const { return m_impl ? m_impl->file_handle() : INVALID_HANDLE_VALUE; }
#endif

    AnonymousBuffer() = default;

    bool is_valid() const { return m_impl; }

    size_t size() const { return m_impl ? m_impl->size() : 0; }

    template<typename T>
    T* data()
    {
        static_assert(IsVoid<T> || IsTrivial<T>);
        if (!m_impl)
            return nullptr;
        return (T*)m_impl->data();
    }

    template<typename T>
    T const* data() const
    {
        static_assert(IsVoid<T> || IsTrivial<T>);
        if (!m_impl)
            return nullptr;
        return (T const*)m_impl->data();
    }

private:
    explicit AnonymousBuffer(NonnullRefPtr<AnonymousBufferImpl>);

    RefPtr<AnonymousBufferImpl> m_impl;
};

}

namespace IPC {

template<>
ErrorOr<void> encode(Encoder&, Core::AnonymousBuffer const&);

template<>
ErrorOr<Core::AnonymousBuffer> decode(Decoder&);

}
