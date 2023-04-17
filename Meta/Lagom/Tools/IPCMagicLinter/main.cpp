/*
 * Copyright (c) 2021, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>
 * Copyright (c) 2023, Cameron Youell <cameronyouell@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/HashMap.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Vector.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibMain/Main.h>

// Exit code is bitwise-or of these values:
static constexpr auto EXIT_COLLISION = 0x1;
static constexpr auto EXIT_ERROR = 0x2;

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    Vector<StringView> ipc_files;

    Core::ArgsParser args_parser;
    args_parser.add_positional_argument(ipc_files, "IPC files to check", "ipc_files", Core::ArgsParser::Required::Yes);
    args_parser.parse(arguments);

    // Read files, compute their hashes, ignore collisions for now.
    HashMap<u32, Vector<DeprecatedString>> inverse_hashes;
    bool had_errors = false;
    for (auto const& filename : ipc_files) {
        auto file_or_error = Core::File::open(filename, Core::File::OpenMode::Read);

        if (file_or_error.is_error()) {
            warnln("Error: Cannot open '{}': {}", filename, file_or_error.error());
            had_errors = true;
            continue; // next file
        }
        auto file = file_or_error.release_value();
        Optional<String> endpoint_name;

        auto buffer_or_error = file->read_until_eof();
        if (buffer_or_error.is_error()) {
            warnln("Error: Cannot read '{}': {}", filename, buffer_or_error.error());
            had_errors = true;
            continue; // next file
        }
        auto buffer = buffer_or_error.release_value();

        size_t length = 0;
        while (true) {
            StringBuilder builder;

            if (length >= buffer.size())
                break;

            while (true) {
                auto current = buffer.bytes()[length];
                ++length;

                if (current == '\n' || current == '\r')
                    break;

                builder.append((char)current);
            }

            auto line_or_error = builder.to_string();
            if (line_or_error.is_error()) {
                warnln("Error: Cannot convert line to string: {}", line_or_error.error());
                had_errors = true;
                continue; // next file
            }
            auto line = line_or_error.release_value();

            if (!line.starts_with_bytes("endpoint "sv))
                continue;

            auto line_endpoint_name_or_error = line.substring_from_byte_offset("endpoint "sv.length());
            if (line_endpoint_name_or_error.is_error()) {
                warnln("Error: Cannot extract endpoint name from line: {}", line_endpoint_name_or_error.error());
                had_errors = true;
                continue; // next file
            }
            auto line_endpoint_name = line_endpoint_name_or_error.release_value();

            if (endpoint_name.has_value()) {
                // Note: If there are three or more endpoints defined in a file, these errors will look a bit wonky.
                // However, that's fine, because it shouldn't happen in the first place.
                warnln("Error: Multiple endpoints in file '{}': Found {} and {}", filename, endpoint_name.value(), line_endpoint_name);
                had_errors = true;
                continue; // next line
            }

            auto endpoint_name_or_error = String::from_utf8(line_endpoint_name);
            if (endpoint_name_or_error.is_error()) {
                warnln("Error: Cannot convert endpoint name to string: {}", endpoint_name_or_error.error());
                had_errors = true;
                continue; // next file
            }
            endpoint_name = endpoint_name_or_error.release_value();
        }

        if (!endpoint_name.has_value()) {
            // If this happens, this file probably needs to parse the endpoint name more carefully.
            warnln("Error: Could not detect endpoint name in file '{}'", filename);
            had_errors = true;
            continue; // next file
        }
        u32 hash = endpoint_name.value().hash();
        auto& files_with_hash = inverse_hashes.ensure(hash);
        files_with_hash.append(filename);
    }

    // Report any collisions
    bool had_collisions = false;
    for (auto const& specific_collisions : inverse_hashes) {
        if (specific_collisions.value.size() <= 1)
            continue;
        outln("Collision: Multiple endpoints use the magic number {}:", specific_collisions.key);
        for (auto const& colliding_file : specific_collisions.value) {
            outln("- {}", colliding_file);
        }
        had_collisions = true;
    }

    outln("Checked {} files, saw {} distinct magic numbers.", ipc_files.size(), inverse_hashes.size());
    if (had_collisions)
        outln("Consider giving your new service a different name.");

    if (had_errors)
        warnln("Some errors were encountered. There may be endpoints with colliding magic numbers.");

    int exit_code = 0;
    if (had_collisions)
        exit_code |= EXIT_COLLISION;
    if (had_errors)
        exit_code |= EXIT_ERROR;
    return exit_code;
}
