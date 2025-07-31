Write-Host -ForegroundColor Green "`n## Build Path $env:Build_Repository_LocalPath #####`n"

$build_encryption = "MBEDTLS"

if ($env:CC_SHORTNAME -eq "vs2008" -or $env:CC_SHORTNAME -eq "vs2013") {
    # on VS2008 mbedtls can not be built since it includes stdint.h which is not available there
    $build_encryption = "OFF"
    Write-Host -ForegroundColor Green "`n## Building without encryption on VS2008 or VS2013 #####`n"
}

if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
    # Workaround for CMake not wanting sh.exe on PATH for MinGW (necessary for CMake 3.12.2)
    $env:PATH = ($env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\bin' }) -join ';'
    $env:PATH = ($env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\usr\bin' }) -join ';'
    # Add mingw to path so that CMake finds e.g. clang
    $env:PATH = "$env:MSYS2_ROOT\mingw64\bin;$env:PATH"
    
    # Set PKG_CONFIG_PATH for mbedtls
    $env:PKG_CONFIG_PATH = "$env:MSYS2_ROOT\mingw64\lib\pkgconfig"
    
    # For Ninja generator, we need to ensure ninja is in PATH
    if ($env:GENERATOR -eq "Ninja") {
        # Check if ninja is available
        try {
            & ninja --version | Out-Null
            Write-Host -ForegroundColor Green "Ninja found in PATH"
        } catch {
            Write-Host -ForegroundColor Red "Ninja not found in PATH, trying to locate it"
            $env:PATH = "$env:MSYS2_ROOT\mingw64\bin;$env:PATH"
        }
    }
}

$vcpkg_toolchain = ""
$vcpkg_triplet = ""

if ($env:CC_SHORTNAME -eq "mingw") {

} elseif ($env:CC_SHORTNAME -eq "clang-mingw") {
    # Setup clang
    $env:CC = "clang --target=x86_64-w64-mingw32"
    $env:CXX = "clang++ --target=x86_64-w64-mingw32"
    clang --version
} else {
    $vcpkg_toolchain = '-DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"'
    $vcpkg_triplet = '-DVCPKG_TARGET_TRIPLET="x64-windows-static"'
}

# Build cmake configuration command line
$cmake_cmd = "cmake"
if ($vcpkg_toolchain) { $cmake_cmd += " $vcpkg_toolchain" }
if ($vcpkg_triplet) { $cmake_cmd += " $vcpkg_triplet" }

# Handle generator and architecture
if ($env:GENERATOR -like "*Visual Studio*" -and $env:ARCH) {
    $cmake_cmd += " -G `"$env:GENERATOR`" -A $env:ARCH"
} else {
    $cmake_cmd += " -G `"$env:GENERATOR`""
}

# Add CMAKE_PREFIX_PATH for MinGW builds to find mbedtls
if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
    $cmake_cmd += " -DCMAKE_PREFIX_PATH=`"$env:MSYS2_ROOT\mingw64`""
}

# Fix MSVC runtime library conflicts for Debug builds
if ($env:GENERATOR -like "*Visual Studio*") {
    $cmake_cmd += " -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug"
}

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with unit tests #####`n"
New-Item -ItemType directory -Path "build"
cd build

# For MinGW builds, use conservative flags to avoid MbedTLS PSA crypto API issues
if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
    $werror_setting = "OFF"
    Write-Host -ForegroundColor Yellow "Disabling UA_FORCE_WERROR for MinGW due to MbedTLS PSA crypto API conflicts"
} else {
    $werror_setting = "ON"
}

# Remove the CFLAGS settings since they don't work with CMake flag ordering
# if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
#     $env:CFLAGS = "-Wno-error=redundant-decls"
#     $env:CXXFLAGS = "-Wno-error=redundant-decls"
# }

$cmake_full_cmd = "$cmake_cmd -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=OFF -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_ALLOW_REUSEADDR=ON -DUA_ENABLE_DA=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_ENCRYPTION:STRING=$build_encryption -DUA_ENABLE_JSON_ENCODING:BOOL=ON -DUA_ENABLE_PUBSUB:BOOL=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON -DUA_FORCE_WERROR=$werror_setting .."
Write-Host -ForegroundColor Yellow "Executing: $cmake_full_cmd"
Invoke-Expression $cmake_full_cmd
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}

Write-Host -ForegroundColor Green "`n### Running unit tests ###`n"
& cmake --build . --target test-verbose -j 1
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Test failed. Exiting ... ***"
    exit $LASTEXITCODE
} else {
    Write-Host -ForegroundColor Green "`n### Unit tests completed successfully ###`n"
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Building $env:CC_NAME examples #####`n"
New-Item -ItemType directory -Path "build"
cd build
$cmake_full_cmd = "$cmake_cmd -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES:BOOL=ON -DUA_FORCE_WERROR=ON .."
Invoke-Expression $cmake_full_cmd
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make install failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with full NS0 #####`n"
New-Item -ItemType directory -Path "build"
cd build
$cmake_full_cmd = "$cmake_cmd -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE=Debug -DUA_ENABLE_DA:BOOL=ON -DUA_ENABLE_JSON_ENCODING:BOOL=ON -DUA_ENABLE_PUBSUB:BOOL=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON -DUA_ENABLE_SUBSCRIPTIONS_EVENTS:BOOL=ON -DUA_FORCE_WERROR=ON -DUA_NAMESPACE_ZERO:STRING=FULL .."
Invoke-Expression $cmake_full_cmd
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME (.dll) #####`n"
New-Item -ItemType directory -Path "build"
cd build
$cmake_full_cmd = "$cmake_cmd -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE=Debug -DUA_FORCE_WERROR=ON .."
Invoke-Expression $cmake_full_cmd
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force
