$ErrorActionPreference = "Stop"

$ARG0 = $MyInvocation.MyCommand.Path


function print_help {
    $NAME = (Split-Path -Leaf $ARG0)
	@"
Usage: $NAME COMMAND [TARGET] [TOOLCHAIN] [ARGS...]
  Supported TARGETs: aarch64, x86_64, lagom. Defaults to SERENITY_ARCH, or x86_64 if not set.
  Supported TOOLCHAINs: GNU, Clang. Defaults to SERENITY_TOOLCHAIN, or GNU if not set.
  Supported COMMANDs:
    build:      Compiles the target binaries, [ARGS...] are passed through to ninja
    install:    Installs the target binary
    image:      Creates a disk image with the installed binaries
    run:        TARGET lagom: $NAME run lagom LAGOM_EXECUTABLE [ARGS...]
                    Runs the Lagom-built LAGOM_EXECUTABLE on the build host, e.g.
                    'shell' or 'js', [ARGS...] are passed through to the executable
                All other TARGETs: $NAME run [TARGET] [KERNEL_CMD_LINE]
                    Runs the built image in QEMU, and optionally passes the
                    KERNEL_CMD_LINE to the Kernel
    gdb:        Same as run, but also starts a gdb remote session.
                TARGET lagom: $NAME gdb lagom LAGOM_EXECUTABLE [-ex 'any gdb command']...
                    Passes through '-ex' commands to gdb
                All other TARGETs: $NAME gdb [TARGET] [KERNEL_CMD_LINE] [-ex 'any gdb command']...
                    If specified, passes the KERNEL_CMD_LINE to the Kernel
                    Passes through '-ex' commands to gdb
    test:       TARGET lagom: $NAME test lagom [TEST_NAME_PATTERN]
                    Runs the unit tests on the build host, or if TEST_NAME_PATTERN
                    is specified tests matching it.
                All other TARGETs: $NAME test [TARGET]
                    Runs the built image in QEMU in self-test mode, by passing
                    system_mode=self-test to the Kernel
    delete:     Removes the build environment for TARGET
    recreate:   Deletes and re-creates the build environment for TARGET
    rebuild:    Deletes and re-creates the build environment, and compiles for TARGET
    kaddr2line: $NAME kaddr2line TARGET ADDRESS
                    Resolves the ADDRESS in the Kernel/Kernel binary to a file:line
    addr2line:  $NAME addr2line TARGET BINARY_FILE ADDRESS
                    Resolves the ADDRESS in BINARY_FILE to a file:line. It will
                    attempt to find the BINARY_FILE in the appropriate build directory
    rebuild-toolchain: Deletes and re-builds the TARGET's toolchain
    rebuild-world:     Deletes and re-builds the toolchain and build environment for TARGET.
    copy-src:   Same as image, but also copies the project's source tree to ~/Source/serenity
                in the built disk image.


	Examples:
	$NAME run x86_64 GNU smp=on
		Runs the image in QEMU passing `"smp=on`" to the kernel command line
	$NAME run x86_64 GNU 'init=/bin/UserspaceEmulator init_args=/bin/SystemServer'
		Runs the image in QEMU, and run the entire system through UserspaceEmulator (not fully supported yet)
	$NAME run
		Runs the image for the default TARGET x86_64 in QEMU
	$NAME run lagom js -A
		Runs the Lagom-built js(1) REPL
	$NAME test lagom
		Runs the unit tests on the build host
	$NAME kaddr2line x86_64 0x12345678
		Resolves the address 0x12345678 in the Kernel binary
	$NAME addr2line x86_64 WindowServer 0x12345678
		Resolves the address 0x12345678 in the WindowServer binary
	$NAME gdb x86_64 smp=on -ex 'hb *init'
		Runs the image for the TARGET x86_64 in qemu and attaches a gdb session
		setting a breakpoint at the init() function in the Kernel.
"@
}


function die {
	Write-Error "die: $args"
    exit 1
}

function usage {
	print_help
	exit 1
}

Set-Variable -Name "CMD" -Value $args[0] -Scope Global
if (!$CMD) {
	usage
	exit
}
$args = $args[1..$args.Count]
if ($CMD -eq "help") {
	print_help
	exit 0
}

if (([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
	die "Do not run serenity.ps1 as Administrator, your Build directory will become Administrator-owned"
}

if ($args) {
	Set-Variable -Name "TARGET" -Value $args[0] -Scope Global
	$args = $args[1..$args.Count]
}
else {
	Set-Variable -Name "TARGET" -Value $env:SERENITY_ARCH -Scope Global
	if ($SERENITY_ARCH -eq $null) {
		Set-Variable -Name "TARGET" -Value "x86_64" -Scope Global
	}
}

Set-Variable -Name "CMAKE_ARGS" -Value @() -Scope Global
Set-Variable -Name "HOST_COMPILER" -Value "" -Scope Global
Set-Variable -Name "BUILD_DIR" -Value "" -Scope Global
Set-Variable -Name "SUPER_BUILD_DIR" -Value "" -Scope Global

# Toolchain selection only applies to non-lagom targets.
if ($TARGET -ne "lagom" -and $args) {
	Set-Variable -Name "TOOLCHAIN_TYPE" -Value $args[0] -Scope Global
	$args = $args[1..$args.length]
} else {
	Set-Variable -Name "TOOLCHAIN_TYPE" -Value "GNU" -Scope Global
	if ($env:SERENITY_TOOLCHAIN -ne $null) {
		$TOOLCHAIN_TYPE = $env:SERENITY_TOOLCHAIN
	}
}

if ($TOOLCHAIN_TYPE -notmatch "^(GNU|Clang)$") {
	Write-Error "ERROR: unknown toolchain '$TOOLCHAIN_TYPE'."
	exit 1
}

$global:CMAKE_ARGS += ("-DSERENITY_TOOLCHAIN=$TOOLCHAIN_TYPE")
Set-Variable -Name "CMD_ARGS" -Value $args -Scope Global

function get_top_dir {
	git rev-parse --show-toplevel
}

function is_valid_target() {
    if ($TARGET -eq "aarch64") {
        $global:CMAKE_ARGS += ("-DSERENITY_ARCH=aarch64")
        return $true
    }
    if ($TARGET -eq "x86_64") {
        $global:CMAKE_ARGS += ("-DSERENITY_ARCH=x86_64")
        return $true
    }
    if ($TARGET -eq "lagom") {
        $global:CMAKE_ARGS += ("-DBUILD_LAGOM=ON")
        if ($global:CMD_ARGS[0] -eq "ladybird") {
            $global:CMAKE_ARGS += ("-DENABLE_LAGOM_LADYBIRD=ON")
        }
        return $true
    }
    return $false
}

function create_build_dir {
    Write-Host "Creating build directory: $env:SERENITY_SOURCE_DIR/Meta/Lagom -B $global:SUPER_BUILD_DIR"
    if ($TARGET -ne "lagom") {
        cmake -GNinja $global:CMAKE_ARGS -S "$env:SERENITY_SOURCE_DIR/Meta/CMake/Superbuild" -B $global:SUPER_BUILD_DIR
    }
    else {	
        cmake -GNinja $global:CMAKE_ARGS -S "$env:SERENITY_SOURCE_DIR/Meta/Lagom" -B $global:SUPER_BUILD_DIR
    }
}

function is_supported_compiler($compiler) {

    if (-not $compiler) {
        return $false
    }

    $version = ""
    try {
		$version = & $compiler -dumpversion
    } catch {
        return $false
    }

    $major_version = [int]($version -split "\.")[0]
    if (& $compiler --version 2>&1 | Select-String "Apple clang") {
        # Apple Clang version check
        if ($major_version -ge 14) {
            return $true
        }
    } elseif (& $compiler --version 2>&1 | Select-String "clang") {
        # Clang version check
        if ($major_version -ge 13) {
            return $true
        }
    } else {
        # GCC version check
        if ($major_version -ge 12) {
            return $true
        }
    }

    return $false
}

function find_newest_compiler() {
	$BEST_VERSION=0
	$BEST_CANDIDATE=""

	foreach ($CANDIDATE in $args) {
		if (!(Get-Command $CANDIDATE -ErrorAction SilentlyContinue)) {
			continue
		}
		if (!(&$CANDIDATE -dumpversion 2>&1)) {
			continue
		}

		$VERSION=""
		$VERSION=&$CANDIDATE -dumpversion
		$MAJOR_VERSION=$VERSION.Split('.')[0]
		if ($MAJOR_VERSION -gt $BEST_VERSION) {
			$BEST_VERSION=$MAJOR_VERSION
			$BEST_CANDIDATE=$CANDIDATE
		}
	}

	$HOST_COMPILER=$BEST_CANDIDATE
	Set-Variable -Name HOST_COMPILER -Value $BEST_CANDIDATE -Scope Global
}

function pick_host_compiler {
    if ((is_supported_compiler $env:CC) -and (is_supported_compiler $env:CXX)) {
        return
    }

	find_newest_compiler clang clang-13 clang-14 clang-15 /opt/homebrew/opt/llvm/bin/clang
    if (is_supported_compiler $HOST_COMPILER) {
        $env:CC = $HOST_COMPILER
        $env:CXX = $HOST_COMPILER.Replace("clang", "clang++")
        return
    }

    find_newest_compiler egcc gcc gcc-12 /usr/local/bin/gcc-12 /opt/homebrew/bin/gcc-12
    if (is_supported_compiler $HOST_COMPILER) {
        $env:CC = $HOST_COMPILER
        $env:CXX = $HOST_COMPILER.Replace("gcc", "g++")
        return
    }

    die "Please make sure that GCC version 12, Clang version 13, or higher is installed."
}

function cmd_with_target() {
	if (!(is_valid_target)) {Write-Error "Unknown target: $env:TARGET"; usage}

	pick_host_compiler
	$global:CMAKE_ARGS += @("-DCMAKE_C_COMPILER=$CC")
	$global:CMAKE_ARGS += @("-DCMAKE_CXX_COMPILER=$CXX")

	if (!(Test-Path "$env:SERENITY_SOURCE_DIR")) {
		$env:SERENITY_SOURCE_DIR = $(get_top_dir)
	}
	$env:TARGET_TOOLCHAIN = ""
	if (($TOOLCHAIN_TYPE -ne "GNU") -and ($TARGET -ne "lagom")) {
		# Only append the toolchain if it's not GNU
		Set-Item -Path env:TARGET_TOOLCHAIN -Value $(echo "-$TARGET_TOOLCHAIN" | tr "[:upper:]" "[:lower:]")
	}
	$global:BUILD_DIR = "$env:SERENITY_SOURCE_DIR/Build/$TARGET$TARGET_TOOLCHAIN"
	if ($TARGET -ne "lagom") {
		Set-Item -Path env:SERENITY_ARCH -Value $TARGET
		Set-Item -Path env:SERENITY_TOOLCHAIN -Value $TOOLCHAIN_TYPE
		if ($TOOLCHAIN_TYPE -eq "Clang") {
			Set-Item -Path env:TOOLCHAIN_DIR -Value "$env:SERENITY_SOURCE_DIR/Toolchain/Local/clang"
		} else {
			Set-Item -Path env:TOOLCHAIN_DIR -Value "$env:SERENITY_SOURCE_DIR/Toolchain/Local/$TARGET_TOOLCHAIN/$TARGET"
		}
		$global:SUPER_BUILD_DIR = "$env:SERENITY_SOURCE_DIR/Build/superbuild-$TARGET$TARGET_TOOLCHAIN"
	} else {
		$global:SUPER_BUILD_DIR = "$BUILD_DIR"		
		$global:CMAKE_ARGS += @("-DCMAKE_INSTALL_PREFIX=$env:SERENITY_SOURCE_DIR/Build/lagom-install")
		$global:CMAKE_ARGS += @("-DSERENITY_CACHE_DIR=${env:SERENITY_SOURCE_DIR}/Build/caches")
	}

	Set-Item -Path env:PATH -Value "$env:SERENITY_SOURCE_DIR/Toolchain/Local/cmake/bin;$env:PATH"
}

function ensure_target {
    if (!(Test-Path "$global:SUPER_BUILD_DIR/build.ninja")) {
        create_build_dir
    }
}

function run_tests {
    param([string]$TEST_NAME)

    $env:CTEST_OUTPUT_ON_FAILURE=1
    if ($TEST_NAME) {
        Set-Location $BUILD_DIR
        cmd /c "ctest -R $TEST_NAME"
    } else {
        Set-Location $BUILD_DIR
        cmd /c "ctest"
    }
}

function build_target {
    if ($TARGET -eq "lagom") {
		$EXTRA_CMAKE_ARGS = ""
		if ($global:CMD_ARGS[0] -eq "ladybird") {
			$EXTRA_CMAKE_ARGS = "-DENABLE_LAGOM_LADYBIRD=ON"
		}

		cmake -S "$env:SERENITY_SOURCE_DIR/Meta/Lagom" -B "$BUILD_DIR" -DBUILD_LAGOM=ON $EXTRA_CMAKE_ARGS
	}
	
	if (!$env:MAKEJOBS) {
		$env:MAKEJOBS = (cmake -P "$env:SERENITY_SOURCE_DIR/Meta/CMake/processor-count.cmake")
	}
	
	if ($args.Count -eq 0) {
		$env:CMAKE_BUILD_PARALLEL_LEVEL="$env:MAKEJOBS"
		cmake --build "$global:SUPER_BUILD_DIR"
	} else {
		ninja -j "$env:MAKEJOBS" -C "$BUILD_DIR" -- $args
	}
}

function build_image {
	Write-Host "NOT IMPLEMENTED: build_image"
}

function delete_target {
    if(Test-Path $global:BUILD_DIR) { Remove-Item $global:BUILD_DIR -Recurse -Force }
    if(Test-Path $global:SUPER_BUILD_DIR) { Remove-Item $global:SUPER_BUILD_DIR -Recurse -Force }
}

function build_cmake {
	Write-Host "NOT IMPLEMENTED: build_cmake"
}

function build_toolchain {
	Write-Host "build_toolchain: $env:TOOLCHAIN_DIR"
	if ($env:TOOLCHAIN_TYPE -eq "Clang") {
		pwsh -Command {
			Set-Location "$env:SERENITY_SOURCE_DIR/Toolchain";
			.\BuildClang.ps1;
		}
	}
	else {
		pwsh -Command {
			Set-Location "$env:SERENITY_SOURCE_DIR/Toolchain";
			$env:ARCH="$env:TARGET";
			.\BuildIt.ps1
		} 
	}
}

function ensure_toolchain {
	if ((cmake -P "$env:SERENITY_SOURCE_DIR/Meta/CMake/cmake-version.cmake") -ne 1) {
		build_cmake
	}
	
	if (!(Test-Path "$env:TOOLCHAIN_DIR")) {
		build_toolchain
	}
	
	if ($env:TOOLCHAIN_TYPE -eq "GNU") {
		$ld_version = & "$env:TOOLCHAIN_DIR/bin/$env:TARGET-pc-serenity-ld" -v
		$expected_version = "GNU ld (GNU Binutils) 2.39"
		if ($ld_version -ne $expected_version) {
			Write-Host "Your toolchain has an old version of binutils installed."
			Write-Host "    installed version: `"$ld_version`""
			Write-Host "    expected version:  `"$expected_version`""
			Write-Host "Please run $env:ARG0 rebuild-toolchain $env:TARGET to update it."
			exit 1
		}
	}
}

function confirm_rebuild_if_toolchain_exists {
	Write-Host "NOT IMPLEMENTED: confirm_rebuild_if_toolchain_exists"
}

function delete_toolchain {
	Write-Host "NOT IMPLEMENTED: delete_toolchain"
}

function kill_tmux_session {
	Write-Host "NOT IMPLEMENTED: kill_tmux_session"
}

function set_tmux_title {
	Write-Host "NOT IMPLEMENTED: set_tmux_title"
}

function lagom_unsupported {
	Write-Host "NOT IMPLEMENTED: lagom_unsupported"
}

function run_gdb {
	Write-Host "NOT IMPLEMENTED: run_gdb"
}

function build_and_run_lagom_target {
    param (
        [string]$run_target,
        [string]$lagom_target = $args[0]
    )

    $lagom_args = ""
    $cmd_args = $args[1..$args.Length] -replace ";", "\\;"
    $lagom_args = $cmd_args -join ";"

	Set-Item -Path env:LAGOM_TARGET -Value $lagom_target
	Set-Item -Path env:LAGOM_ARGS -Value $lagom_args
    build_target $run_target
}

if ($CMD -match "^(build|install|image|copy-src|run|gdb|test|rebuild|recreate|kaddr2line|addr2line|setup-and-run)$") {
    cmd_with_target
    if ($CMD -ne "recreate" -and $CMD -ne "rebuild") {
		delete_target
    }
    if ($TARGET -ne "lagom") {
		ensure_toolchain
    }
    ensure_target
    switch ($CMD) {
        "build" {
            build_target @($args)
        }
        "install" {
            build_target
            build_target install
        }
        "image" {
            lagom_unsupported
            build_target
            build_target install
            build_image
        }
        "copy-src" {
            lagom_unsupported
            build_target
            build_target install
            $env:SERENITY_COPY_SOURCE=1
            build_image
        }
        "run" {
            if ($TARGET -eq "lagom") {
                if ($CMD_ARGS[0] -eq "ladybird") {
                    build_and_run_lagom_target "run-ladybird"
                }
                else {
                    build_and_run_lagom_target "run-lagom-target"
                }
            }
            else {
                build_target
                build_target install
                build_image
                if ($CMD_ARGS[0]) {
                    $env:SERENITY_KERNEL_CMDLINE=$CMD_ARGS[0]
                }
                build_target run
            }
        }
        "gdb" {
            if (!(Test-Path (Get-Command tmux))) {
                die "Please install tmux!"
            }
            if ($TARGET -eq "lagom") {
                if ($args.Count -lt 1) {
                    usage
                }
                build_target @($args)
                run_gdb $CMD_ARGS
            }
            else {
                build_target
                build_target install
                build_image
                tmux new-session $ARG0 __tmux_cmd $TARGET $TOOLCHAIN_TYPE run $CMD_ARGS `; set-option -t 0 mouse on `; split-window $ARG0 __tmux_cmd $TARGET $TOOLCHAIN_TYPE gdb $CMD_ARGS `;
            }
        }
        "test" {
            build_target
            if ($TARGET -eq "lagom") {
                run_tests $CMD_ARGS[0]
            }
            else {
                build_target install
                build_image
                $env:SERENITY_KERNEL_CMDLINE="graphics_subsystem_mode=off system_mode=self-test"
                $env:SERENITY_RUN="ci"
                build_target run
            }
        }
        "rebuild" {
            build_target @($args)
        }
        "recreate" {
            # do nothing
        }
        "kaddr2line" {
            lagom_unsupported
            build_target
            if ($args.Count -lt 1) {
                usage
            }
            if ($TOOLCHAIN_TYPE -eq "Clang") {
                $ADDR2LINE="$TOOLCHAIN_DIR/bin/llvm-addr2line"
            }
            else {
                $ADDR2LINE="$TOOLCHAIN_DIR/bin/$TARGET-pc-serenity-addr2line"
            }
            & $ADDR2LINE -e "$BUILD_DIR/Kernel/Kernel" @($args)
        }
		"addr2line" {
			build_target
			if ($args.Count -ge 2) { usage }
			$BINARY_FILE = $args[0]
			$args = $args[1..$args.Count]
			$BINARY_FILE_PATH = "$BUILD_DIR/$BINARY_FILE"
			if ($TARGET -eq "lagom") {
				if (test-path "addr2line") { die "Please install addr2line!" }
				$ADDR2LINE = "addr2line"
			}
			elseif ($TOOLCHAIN_TYPE -eq "Clang") {
				$ADDR2LINE = "$TOOLCHAIN_DIR/bin/llvm-addr2line"
			}
			else {
				$ADDR2LINE = "$TOOLCHAIN_DIR/bin/$TARGET-pc-serenity-addr2line"
			}
			if (test-path "$BINARY_FILE_PATH") {
				& $ADDR2LINE -e "$BINARY_FILE_PATH" $args
			}
			else {
				get-childitem -path "$BUILD_DIR" -name "$BINARY_FILE" -executable -type file | % { & $ADDR2LINE -e $_.FullName $args }
			}
		}
		default {
			build_target "$CMD" $args
		}
	}
} elseif ($CMD -eq "delete") {
    cmd_with_target
    delete_target
} elseif ($CMD -eq "rebuild-toolchain") {
    cmd_with_target
    lagom_unsupported "The lagom target uses the host toolchain"
    confirm_rebuild_if_toolchain_exists
    delete_toolchain
    ensure_toolchain
} elseif ($CMD -eq "rebuild-world") {
    cmd_with_target
    lagom_unsupported "The lagom target uses the host toolchain"
    delete_toolchain
    delete_target
    ensure_toolchain
    ensure_target
    build_target
} elseif ($CMD -eq "__tmux_cmd") {
    trap {kill_tmux_session} EXIT
    cmd_with_target
    $CMD = $args[0]; shift
    $CMD_ARGS = @($CMD_ARGS[1..($CMD_ARGS.Count-1)])
    if ($CMD -eq "run") {
        if ($CMD_ARGS[0] -ne "") {
            $env:SERENITY_KERNEL_CMDLINE = $CMD_ARGS[0]
        }
        # We need to make sure qemu doesn't start until we continue in gdb
        $env:SERENITY_EXTRA_QEMU_ARGS = "$env:SERENITY_EXTRA_QEMU_ARGS -d int -no-reboot -no-shutdown -S"
        # We need to disable kaslr to let gdb map the kernel symbols correctly
        $env:SERENITY_KERNEL_CMDLINE = "$env:SERENITY_KERNEL_CMDLINE disable_kaslr"
        set_tmux_title 'qemu'
        build_target run
    } elseif ($CMD -eq "gdb") {
        set_tmux_title 'gdb'
        run_gdb $CMD_ARGS
    } else {
        Write-Error "Unknown command: $CMD"
        usage
    }
}
