/*
 * Copyright (c) 2022, Stephan Unverwerth <s.unverwerth@serenityos.org>
 * Copyright (c) 2023, Jelle Raaijmakers <jelle@gmta.nl>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/HashMap.h>
#include <AK/WeakPtr.h>
#include <LibGPU/Driver.h>
#if !defined(AK_OS_WINDOWS)
#    include <dlfcn.h>
#endif

namespace GPU {

// FIXME: Think of a better way to configure these paths. Maybe use ConfigServer?
// clang-format off
static HashMap<StringView, char const*> const s_driver_path_map
{
#if defined(AK_OS_SERENITY)
    { "softgpu"sv, "libsoftgpu.so.serenity" },
    { "virtgpu"sv, "libvirtgpu.so.serenity" },
#elif defined(AK_OS_MACOS)
    { "softgpu"sv, "liblagom-softgpu.dylib" },
#else
    { "softgpu"sv, "liblagom-softgpu.so.0" },
#endif
};
// clang-format on

static HashMap<DeprecatedString, WeakPtr<Driver>> s_loaded_drivers;

ErrorOr<NonnullRefPtr<Driver>> Driver::try_create(StringView driver_name)
{
    // Check if the library for this driver is already loaded
    auto already_loaded_driver = s_loaded_drivers.find(driver_name);
    if (already_loaded_driver != s_loaded_drivers.end() && !already_loaded_driver->value.is_null())
        return *already_loaded_driver->value;

    // Nope, we need to load the library
    auto it = s_driver_path_map.find(driver_name);
    if (it == s_driver_path_map.end())
        return Error::from_string_literal("The requested GPU driver was not found in the list of allowed driver libraries");

#if defined(AK_OS_WINDOWS)
    auto lib = LoadLibraryA(it->value);
#else
    auto lib = dlopen(it->value, RTLD_NOW);
#endif
    if (!lib)
        return Error::from_string_literal("The library for the requested GPU driver could not be opened");

#if defined(AK_OS_WINDOWS)
    auto serenity_gpu_create_device = reinterpret_cast<serenity_gpu_create_device_t>(GetProcAddress(lib, "serenity_gpu_create_device"));
    if (!serenity_gpu_create_device) {
        FreeLibrary(lib);
        return Error::from_string_literal("The library for the requested GPU driver does not contain serenity_gpu_create_device()");
    }
#else
    auto serenity_gpu_create_device = reinterpret_cast<serenity_gpu_create_device_t>(dlsym(lib, "serenity_gpu_create_device"));
    if (!serenity_gpu_create_device) {
        dlclose(lib);
        return Error::from_string_literal("The library for the requested GPU driver does not contain serenity_gpu_create_device()");
    }
#endif

    auto driver = adopt_ref(*new Driver(lib, serenity_gpu_create_device));

    s_loaded_drivers.set(driver_name, driver->make_weak_ptr());

    return driver;
}

Driver::~Driver()
{
#if defined(AK_OS_WINDOWS)
    FreeLibrary(m_dlopen_result);
#else
    dlclose(m_dlopen_result);
#endif
}

ErrorOr<NonnullOwnPtr<Device>> Driver::try_create_device(Gfx::IntSize size)
{
    auto device_or_null = m_serenity_gpu_create_device(size);
    if (!device_or_null)
        return Error::from_string_literal("Could not create GPU device");

    return adopt_own(*device_or_null);
}

}
