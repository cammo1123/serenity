/*
 * Copyright (c) 2021, Nick Vella <nick@nxk.io>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ProjectTemplate.h"
#include <AK/DeprecatedString.h>
#include <AK/LexicalPath.h>
#include <AK/StringBuilder.h>
#include <LibCore/ConfigFile.h>
#include <LibCore/DirIterator.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace HackStudio {

ProjectTemplate::ProjectTemplate(DeprecatedString const& id, DeprecatedString const& name, DeprecatedString const& description, const GUI::Icon& icon, int priority)
    : m_id(id)
    , m_name(name)
    , m_description(description)
    , m_icon(icon)
    , m_priority(priority)
{
}

RefPtr<ProjectTemplate> ProjectTemplate::load_from_manifest(DeprecatedString const& manifest_path)
{
    auto maybe_config = Core::ConfigFile::open(manifest_path);
    if (maybe_config.is_error())
        return {};
    auto config = maybe_config.release_value();

    if (!config->has_group("HackStudioTemplate")
        || !config->has_key("HackStudioTemplate", "Name")
        || !config->has_key("HackStudioTemplate", "Description")
        || !config->has_key("HackStudioTemplate", "IconName32x"))
        return {};

    auto id = LexicalPath::title(manifest_path);
    auto name = config->read_entry("HackStudioTemplate", "Name");
    auto description = config->read_entry("HackStudioTemplate", "Description");
    int priority = config->read_num_entry("HackStudioTemplate", "Priority", 0);

    // Attempt to read in the template icons
    // Fallback to a generic executable icon if one isn't found
    auto icon = GUI::Icon::default_icon("filetype-executable"sv);

    auto bitmap_path_32 = DeprecatedString::formatted("/res/icons/hackstudio/templates-32x32/{}.png", config->read_entry("HackStudioTemplate", "IconName32x"));

    if (FileSystem::exists(bitmap_path_32)) {
        auto bitmap_or_error = Gfx::Bitmap::load_from_file(bitmap_path_32);
        if (!bitmap_or_error.is_error())
            icon = GUI::Icon(bitmap_or_error.release_value());
    }

    return adopt_ref(*new ProjectTemplate(id, name, description, icon, priority));
}

ErrorOr<void> ProjectTemplate::create_project(StringView name, StringView path)
{
    // Check if a file or directory already exists at the project path
    if (FileSystem::exists(path))
        return Error::from_string_view(TRY(String::formatted("A file or directory already exists at path '{}'", path)));

    dbgln("Creating project at path '{}' with name '{}'", path, name);

    // Verify that the template content directory exists. If it does, copy it's contents.
    // Otherwise, create an empty directory at the project path.
    if (FileSystem::is_directory(content_path())) {
        dbgln("Copying {} -> {}", content_path(), path);
        TRY(FileSystem::copy_file_or_directory(path, content_path()));
    } else {
        dbgln("No template content directory found for '{}', creating an empty directory for the project.", m_id);
        TRY(Core::System::mkdir(path, 0755));
    }

    // Check for an executable post-create script in $TEMPLATES_DIR/$ID.postcreate,
    // and run it with the path and name

    auto postcreate_script_path = LexicalPath::canonicalized_path(DeprecatedString::formatted("{}/{}.postcreate", templates_path(), m_id));
    struct stat postcreate_st;
    int result = stat(postcreate_script_path.characters(), &postcreate_st);
    if (result == 0 && (postcreate_st.st_mode & S_IXOTH) == S_IXOTH) {
        dbgln("Running post-create script '{}'", postcreate_script_path);

        // Generate a namespace-safe project name (replace hyphens with underscores)
        auto namespace_safe = name.replace("-"sv, "_"sv, ReplaceMode::All);

        char const* argv[] = { postcreate_script_path.characters(), name.characters_without_null_termination(), path.characters_without_null_termination(), namespace_safe.characters(), nullptr };
        auto child_pid = TRY(Core::System::posix_spawn(postcreate_script_path, nullptr, nullptr, const_cast<char**>(argv), environ));

        // Command spawned, wait for exit.
        auto result = TRY(Core::System::waitpid(child_pid));

        int child_error = WEXITSTATUS(result.status);
        dbgln("Post-create script exited with code {}", child_error);

        if (child_error != 0)
            return Error::from_string_view(TRY(String::formatted("Post-create script exited with code {}", child_error)));
    }

    return {};
}

}
