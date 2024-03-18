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
    [System.Environment]::SetEnvironmentVariable('Path', $path, 'Machine')
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

$cmake_cnf="$vcpkg_toolchain", "$vcpkg_triplet", "-G`"$env:GENERATOR`""

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with unit tests #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake $cmake_cnf `
        -DBUILD_SHARED_LIBS:BOOL=OFF `
        -DCMAKE_BUILD_TYPE=Debug `
        -DUA_BUILD_EXAMPLES=OFF `
        -DUA_BUILD_UNIT_TESTS=ON `
        -DUA_ENABLE_ALLOW_REUSEADDR=ON `
        -DUA_ENABLE_DA=ON `
        -DUA_ENABLE_DISCOVERY=ON `
        -DUA_ENABLE_ENCRYPTION:STRING=$build_encryption `
        -DUA_ENABLE_JSON_ENCODING:BOOL=ON `
        -DUA_ENABLE_PUBSUB:BOOL=ON `
        -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON `
        -DUA_ENABLE_PUBSUB_MONITORING:BOOL=ON `
        -DUA_FORCE_WERROR=ON `
        ..
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}
& cmake --build . --target test-verbose -j 1
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Test failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Building $env:CC_NAME examples #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake $cmake_cnf `
        -DBUILD_SHARED_LIBS:BOOL=OFF `
        -DCMAKE_BUILD_TYPE=Debug `
        -DUA_BUILD_EXAMPLES:BOOL=ON `
        -DUA_ENABLE_AMALGAMATION:BOOL=OFF `
        -DUA_FORCE_WERROR=ON `
        ..
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make install failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with amalgamation #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake $cmake_cnf `
        -DBUILD_SHARED_LIBS:BOOL=OFF `
        -DCMAKE_BUILD_TYPE=Debug `
        -DUA_BUILD_EXAMPLES:BOOL=OFF `
        -DUA_ENABLE_AMALGAMATION:BOOL=ON `
        -DUA_ENABLE_SUBSCRIPTIONS_EVENTS:BOOL=ON `
        -DUA_ENABLE_ENCRYPTION:STRING=$build_encryption `
        -DUA_FORCE_WERROR=ON `
        ..
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with full NS0 #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake $cmake_cnf `
        -DBUILD_SHARED_LIBS:BOOL=OFF `
        -DCMAKE_BUILD_TYPE=Debug `
        -DUA_ENABLE_DA:BOOL=ON `
        -DUA_ENABLE_JSON_ENCODING:BOOL=ON `
        -DUA_ENABLE_PUBSUB:BOOL=ON `
        -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON `
        -DUA_ENABLE_SUBSCRIPTIONS_EVENTS:BOOL=ON `
        -DUA_FORCE_WERROR=ON `
        -DUA_NAMESPACE_ZERO:STRING=FULL `
        ..
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
& cmake $cmake_cnf `
        -DBUILD_SHARED_LIBS:BOOL=ON `
        -DCMAKE_BUILD_TYPE=Debug `
        -DUA_FORCE_WERROR=ON `
        ..
& cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force
