
try {


    Write-Host -ForegroundColor Green "`n## Build Path $env:Build_Repository_LocalPath #####`n"

    $vcpkg_toolchain = ""
    $vcpkg_triplet = ""

    if ($env:CC_SHORTNAME -eq "vs2008" -or $env:CC_SHORTNAME -eq "vs2013") {
        # on VS2008 mbedtls can not be built since it includes stdint.h which is not available there
        $build_encryption = "OFF"
        Write-Host -ForegroundColor Green "`n## Building without encryption on VS2008 or VS2013 #####`n"
    } else {
        $build_encryption = "MBEDTLS"
    }

    if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
        # Workaround for CMake not wanting sh.exe on PATH for MinGW (necessary for CMake 3.12.2)
        $env:PATH = ($env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\bin' }) -join ';'
        $env:PATH = ($env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\usr\bin' }) -join ';'
        # Add mingw to path so that CMake finds e.g. clang
        $env:PATH = "$env:MSYS2_ROOT\mingw64\bin;$env:PATH"
        [System.Environment]::SetEnvironmentVariable('Path', $path, 'Machine')
    }

    if ($env:CC_SHORTNAME -eq "mingw") {

} elseif ($env:CC_SHORTNAME -eq "clang-mingw") {
    # Setup clang
    $env:CC = "clang --target=x86_64-w64-mingw32"
    $env:CXX = "clang++ --target=x86_64-w64-mingw32"
    clang --version
} else {
    $vcpkg_toolchain = '-DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"'
    $vcpkg_triplet = '-DVCPKG_TARGET_TRIPLET="x64-windows"'
    New-Item -Force -ItemType directory -Path "C:/vcpkg/installed/x64-windows"
}

    $cmake_cnf="$vcpkg_toolchain", "$vcpkg_triplet", "-G`"$env:GENERATOR`"", "-DUA_FORCE_CPP:BOOL=$env:FORCE_CXX"

    # Collect files for .zip packing
    New-Item -ItemType directory -Path pack
    Copy-Item LICENSE pack
    Copy-Item AUTHORS pack
    Copy-Item README.md pack

    # Only execute unit tests on vs2019 to save compilation time
    if ($env:CC_SHORTNAME -eq "vs2019") {
        Write-Host -ForegroundColor Green "`n###################################################################"
        Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with unit tests #####`n"
        New-Item -ItemType directory -Path "build"
        cd build
        & cmake $cmake_cnf `
                -DBUILD_SHARED_LIBS:BOOL=OFF `
                -DCMAKE_BUILD_TYPE=Debug `
                -DUA_BUILD_EXAMPLES=OFF `
                -DUA_BUILD_UNIT_TESTS=ON `
                -DUA_ENABLE_DA=ON `
                -DUA_ENABLE_DISCOVERY=ON `
                -DUA_ENABLE_DISCOVERY_MULTICAST=ON `
                -DUA_ENABLE_ENCRYPTION:STRING=$build_encryption `
                -DUA_ENABLE_JSON_ENCODING:BOOL=ON `
                -DUA_ENABLE_PUBSUB:BOOL=ON `
                -DUA_ENABLE_PUBSUB_DELTAFRAMES:BOOL=ON `
                -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON `
                -DUA_ENABLE_PUBSUB_MONITORING:BOOL=ON `
                -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON ..
        & cmake --build . --config Debug
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
        & cmake --build . --target test-verbose --config Debug -j 1
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
        cd ..
        Remove-Item -Path build -Recurse -Force
    }

    Write-Host -ForegroundColor Green "`n###################################################################"
    Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with amalgamation #####`n"
    New-Item -ItemType directory -Path "build"
    cd build
    & cmake $cmake_cnf `
            -DCMAKE_BUILD_TYPE=RelWithDebInfo `
            -DUA_BUILD_EXAMPLES:BOOL=OFF  `
            -DUA_ENABLE_AMALGAMATION:BOOL=ON `
            -DUA_ENABLE_SUBSCRIPTIONS_EVENTS:BOOL=ON `
            -DUA_ENABLE_ENCRYPTION:STRING=$build_encryption ..
    & cmake --build . --config RelWithDebInfo
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
    # Use build type Debug here, to force `-Werror`
    & cmake $cmake_cnf `
            -DCMAKE_BUILD_TYPE=Debug `
            -DUA_BUILD_EXAMPLES:BOOL=ON `
            -DUA_ENABLE_DA:BOOL=ON `
            -DUA_ENABLE_JSON_ENCODING:BOOL=ON `
            -DUA_ENABLE_PUBSUB:BOOL=ON `
            -DUA_ENABLE_PUBSUB_DELTAFRAMES:BOOL=ON `
            -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON `
            -DUA_ENABLE_SUBSCRIPTIONS_EVENTS:BOOL=ON `
            -DUA_NAMESPACE_ZERO:STRING=FULL ..
    & cmake --build . --config Debug
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    cd ..
    Remove-Item -Path build -Recurse -Force

    Write-Host -ForegroundColor Green "`n###################################################################"
    Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME without amalgamation #####`n"
    New-Item -ItemType directory -Path "build"
    cd build
    & cmake $cmake_cnf `
            -DBUILD_SHARED_LIBS:BOOL=OFF `
            -DCMAKE_BUILD_TYPE=RelWithDebInfo `
            -DCMAKE_INSTALL_PREFIX="$env:Build_Repository_LocalPath-$env:CC_SHORTNAME-static" `
            -DUA_BUILD_EXAMPLES:BOOL=ON `
            -DUA_ENABLE_AMALGAMATION:BOOL=OFF ..
    & cmake --build . --target install --config RelWithDebInfo
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0)
    {
        Write-Host -ForegroundColor Red "`n`n*** Make install failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    cd ..
    & 7z a -tzip "$env:Build_ArtifactStagingDirectory/open62541-$env:CC_SHORTNAME-static.zip" "$env:Build_Repository_LocalPath\pack\*" "$env:Build_Repository_LocalPath-$env:CC_SHORTNAME-static\*"
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0)
    {
        Write-Host -ForegroundColor Red "`n`n*** Zipping failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    Remove-Item -Path build -Recurse -Force

    Write-Host -ForegroundColor Green "`n###################################################################"
    Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME (.dll) #####`n"
    New-Item -ItemType directory -Path "build"
    cd build
    & cmake $cmake_cnf `
            -DBUILD_SHARED_LIBS:BOOL=ON `
            -DCMAKE_BUILD_TYPE=RelWithDebInfo `
            -DCMAKE_INSTALL_PREFIX="$env:Build_Repository_LocalPath-$env:CC_SHORTNAME-dynamic" `
            -DUA_BUILD_EXAMPLES:BOOL=ON `
            -DUA_ENABLE_AMALGAMATION:BOOL=OFF ..
    & cmake --build . --target install --config RelWithDebInfo
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0)
    {
        Write-Host -ForegroundColor Red "`n`n*** Make install failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    cd ..
    & 7z a -tzip "$env:Build_ArtifactStagingDirectory/open62541-$env:CC_SHORTNAME-dynamic.zip" "$env:Build_Repository_LocalPath\pack\*" "$env:Build_Repository_LocalPath-$env:CC_SHORTNAME-static\*"
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0)
    {
        Write-Host -ForegroundColor Red "`n`n*** Zipping failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    Remove-Item -Path build -Recurse -Force


    # # do not cache log
    # Remove-Item -Path c:\miktex\texmfs\data\miktex\log -Recurse -Force

} catch {
    # Print a detailed error message
    $FullException = ($_.Exception|format-list -force) | Out-String
    Write-Host -ForegroundColor Red "`n------------------ Exception ------------------`n$FullException`n"
    [Console]::Out.Flush()
    # Wait a bit to make sure appveyor shows the error message
    Start-Sleep 10
    throw
}
