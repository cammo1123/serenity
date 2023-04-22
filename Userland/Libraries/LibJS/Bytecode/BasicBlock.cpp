/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/DeprecatedString.h>
#include <LibCore/System.h>
#include <LibJS/Bytecode/BasicBlock.h>
#include <LibJS/Bytecode/Op.h>

#if !defined(AK_OS_WINDOWS)
#    include <sys/mman.h>
#endif

namespace JS::Bytecode {

NonnullOwnPtr<BasicBlock> BasicBlock::create(DeprecatedString name, size_t size)
{
    return adopt_own(*new BasicBlock(move(name), max(size, static_cast<size_t>(4 * KiB))));
}

BasicBlock::BasicBlock(DeprecatedString name, size_t size)
    : m_name(move(name))
{
    // FIXME: This is not the smartest solution ever. Find something cleverer!
    // The main issue we're working around here is that we don't want pointers into the bytecode stream to become invalidated
    // during code generation due to dynamic buffer resizing. Otherwise we could just use a Vector.
    m_buffer_capacity = size;

#if !defined(AK_OS_WINDOWS)
    m_buffer = (u8*)MUST(Core::System::mmap(nullptr, m_buffer_capacity, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
    VERIFY(m_buffer != MAP_FAILED);
#else
    HANDLE h = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, m_buffer_capacity, nullptr);
    VERIFY(h != nullptr);

    m_buffer = (u8*)MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, m_buffer_capacity);
    VERIFY(m_buffer != nullptr);

    CloseHandle(h);
#endif
}

BasicBlock::~BasicBlock()
{
    Bytecode::InstructionStreamIterator it(instruction_stream());
    while (!it.at_end()) {
        auto& to_destroy = (*it);
        ++it;
        Instruction::destroy(const_cast<Instruction&>(to_destroy));
    }

#if !defined(AK_OS_WINDOWS)
    MUST(Core::System::munmap(m_buffer, m_buffer_capacity));
#else
    UnmapViewOfFile(m_buffer);
#endif
}

void BasicBlock::seal()
{
    // FIXME: mprotect the instruction stream as PROT_READ
    // This is currently not possible because instructions can have destructors (that clean up strings)
    // Instructions should instead be destructor-less and refer to strings in a string table on the Bytecode::Block.
    // It also doesn't work because instructions that have String members use RefPtr internally which must be in writable memory.
}

void BasicBlock::dump(Bytecode::Executable const& executable) const
{
    Bytecode::InstructionStreamIterator it(instruction_stream());
    if (!m_name.is_empty())
        warnln("{}:", m_name);
    while (!it.at_end()) {
        warnln("[{:4x}] {}", it.offset(), (*it).to_deprecated_string(executable));
        ++it;
    }
}

void BasicBlock::grow(size_t additional_size)
{
    m_buffer_size += additional_size;
    VERIFY(m_buffer_size <= m_buffer_capacity);
}

}
