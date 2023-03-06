/*
 * Copyright (c) 2021, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/DeprecatedFile.h>
#include <LibTest/TestCase.h>
#include <unistd.h>

static bool files_have_same_contents(DeprecatedString filename1, DeprecatedString filename2)
{
    auto file1 = Core::DeprecatedFile::open(filename1, Core::OpenMode::ReadOnly).value();
    auto file2 = Core::DeprecatedFile::open(filename2, Core::OpenMode::ReadOnly).value();
    auto contents1 = file1->read_all(), contents2 = file2->read_all();
    return contents1 == contents2;
}

static DeprecatedString get_output_path()
{
#if !defined(AK_OS_WINDOWS)
    return "/tmp/output.txt";
#else
    LPTSTR temp_path_buffer = new TCHAR[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_path_buffer) == 0) {
        warnln("Failed to get temp path");
        VERIFY_NOT_REACHED();
    }
    return DeprecatedString::formatted("{}output.txt", temp_path_buffer);
#endif
}

TEST_CASE(file_readline)
{
    auto path = "long_lines.txt";
    auto file_or_error = Core::DeprecatedFile::open(path, Core::OpenMode::ReadOnly);
    if (file_or_error.is_error()) {
        warnln("Failed to open {}: {}", path, file_or_error.error());
        VERIFY_NOT_REACHED();
    }
    auto file = file_or_error.release_value();
    auto output_path = get_output_path();
    auto outfile_or_error = Core::DeprecatedFile::open(output_path, Core::OpenMode::WriteOnly);
    auto outputfile = outfile_or_error.release_value();
    while (file->can_read_line()) {
        outputfile->write(file->read_line());
        outputfile->write("\n"sv);
    }
    file->close();
    outputfile->close();
    VERIFY(files_have_same_contents(path, output_path));
    unlink(output_path.characters());
}

TEST_CASE(file_get_read_position)
{
    const DeprecatedString path = "10kb.txt";
    auto file = Core::DeprecatedFile::open(path, Core::OpenMode::ReadOnly).release_value();

    const size_t step_size = 98;
    for (size_t i = 0; i < 10240 - step_size; i += step_size) {
        auto read_buffer = file->read(step_size);
        EXPECT_EQ(read_buffer.size(), step_size);

        for (size_t j = 0; j < read_buffer.size(); j++) {
            EXPECT_EQ(static_cast<u32>(read_buffer[j] - '0'), (i + j) % 10);
        }

        off_t offset = 0;
        VERIFY(file->seek(0, SeekMode::FromCurrentPosition, &offset));
        EXPECT_EQ(offset, static_cast<off_t>(i + step_size));
    }

    {
        off_t offset = 0;
        VERIFY(file->seek(0, SeekMode::FromEndPosition, &offset));
        EXPECT_EQ(offset, 10240);
    }

    {
        off_t offset = 0;
        VERIFY(file->seek(0, SeekMode::SetPosition, &offset));
        EXPECT_EQ(offset, 0);
    }
}

TEST_CASE(file_lines_range)
{
    auto path = "long_lines.txt";
    auto file_or_error = Core::DeprecatedFile::open(path, Core::OpenMode::ReadOnly);
    if (file_or_error.is_error()) {
        warnln("Failed to open {}: {}", path, file_or_error.error());
        VERIFY_NOT_REACHED();
    }
    auto file = file_or_error.release_value();
    auto output_path = get_output_path();
    auto outfile_or_error = Core::DeprecatedFile::open(output_path, Core::OpenMode::WriteOnly);
    auto outputfile = outfile_or_error.release_value();
    for (auto line : file->lines()) {
        outputfile->write(line);
        outputfile->write("\n"sv);
    }
    file->close();
    outputfile->close();
    VERIFY(files_have_same_contents(path, output_path));
    unlink(output_path.characters());
}
