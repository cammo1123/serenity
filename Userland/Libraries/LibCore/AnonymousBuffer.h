#pragma once

#include <AK/Error.h>
#include <AK/Noncopyable.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <AK/Windows.h>
#include <LibIPC/Forward.h>

namespace Core {

class AnonymousBufferImpl final : public RefCounted<AnonymousBufferImpl> {
public:
    static ErrorOr<NonnullRefPtr<AnonymousBufferImpl>> create(HANDLE file_handle, size_t);
    ~AnonymousBufferImpl();

    HANDLE file_handle() const { return m_file_handle; }
    size_t size() const { return m_size; }
    void* data() { return m_data; }
    void const* data() const { return m_data; }

private:
    AnonymousBufferImpl(HANDLE file_handle, size_t, void*);

    HANDLE m_file_handle { INVALID_HANDLE_VALUE };
    size_t m_size { 0 };
    void* m_data { nullptr };
};

class AnonymousBuffer {
public:
    static ErrorOr<AnonymousBuffer> create_with_size(size_t);
    static ErrorOr<AnonymousBuffer> create_from_anon_handle(HANDLE file_handle, size_t);

    AnonymousBuffer() = default;

    bool is_valid() const { return m_impl; }

    HANDLE file_handle() const { return m_impl ? m_impl->file_handle() : INVALID_HANDLE_VALUE; }
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
