
try {
    cd $env:APPVEYOR_BUILD_FOLDER

    $vcpkg_toolchain = ""
    $vcpkg_triplet = ""

    if ($env:CC_SHORTNAME -eq "vs2008" -or $env:CC_SHORTNAME -eq "vs2013") {
        # on VS2008 mbedtls can not be built since it includes stdint.h which is not available there
        $build_encryption = "OFF"
        Write-Host -ForegroundColor Green "`n## Building without encryption on VS2008 or VS2013 #####`n"
    } else {
        $build_encryption = "ON"
    }

    if ($env:CC_SHORTNAME -eq "mingw") {

    } elseif ($env:CC_SHORTNAME -eq "clang-mingw") {
        # Setup clang
        $env:CC = "clang --target=x86_64-w64-mingw32"
        $env:CXX = "clang++ --target=x86_64-w64-mingw32"
        clang --version
    } else {
        $vcpkg_toolchain = '-DCMAKE_TOOLCHAIN_FILE="C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake"'
        $vcpkg_triplet = '-DVCPKG_TARGET_TRIPLET="x86-windows-static"'
        # since https://github.com/Microsoft/vcpkg/commit/0334365f516c5f229ff4fcf038c7d0190979a38a#diff-464a170117fa96bf98b2f8d224bf503c
        # vcpkg need to have  "C:\Tools\vcpkg\installed\x86-windows-static"
        New-Item -Force -ItemType directory -Path "C:\Tools\vcpkg\installed\x86-windows-static"
    }

    $cmake_cnf="$vcpkg_toolchain", "$vcpkg_triplet", "-G`"$env:GENERATOR`"", "-DUA_COMPILE_AS_CXX:BOOL=$env:FORCE_CXX"

    # Collect files for .zip packing
    New-Item -ItemType directory -Path pack
    Copy-Item LICENSE pack
    Copy-Item AUTHORS pack
    Copy-Item README.md pack

    Write-Host -ForegroundColor Green "`n###################################################################"
    Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with amalgamation #####`n"
    New-Item -ItemType directory -Path "build"
    cd build
    & cmake $cmake_cnf `
            -DCMAKE_BUILD_TYPE=RelWithDebInfo `
            -DUA_BUILD_EXAMPLES:BOOL=OFF  `
            -DUA_ENABLE_AMALGAMATION:BOOL=ON `
            -DUA_ENABLE_ENCRYPTION:BOOL=$build_encryption ..
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
    & cmake $cmake_cnf `
            -DCMAKE_BUILD_TYPE=RelWithDebInfo `
            -DUA_BUILD_EXAMPLES:BOOL=ON `
            -DUA_ENABLE_DA:BOOL=ON `
            -DUA_ENABLE_JSON_ENCODING:BOOL=ON `
            -DUA_ENABLE_PUBSUB:BOOL=ON `
            -DUA_ENABLE_PUBSUB_DELTAFRAMES:BOOL=ON `
            -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON `
            -DUA_ENABLE_SUBSCRIPTIONS_EVENTS:BOOL=ON `
            -DUA_NAMESPACE_ZERO:STRING=FULL ..
    & cmake --build . --config RelWithDebInfo
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
            -DCMAKE_BUILD_TYPE=RelWithDebInfo `
            -DCMAKE_INSTALL_PREFIX="$env:APPVEYOR_BUILD_FOLDER-$env:CC_SHORTNAME-static" `
            -DUA_BUILD_EXAMPLES:BOOL=ON `
            -DUA_ENABLE_AMALGAMATION:BOOL=OFF ..
    & cmake --build . --target install --config RelWithDebInfo
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0)
    {
        Write-Host -ForegroundColor Red "`n`n*** Make install failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    cd ..
    & 7z a -tzip open62541-$env:CC_SHORTNAME-static.zip "$env:APPVEYOR_BUILD_FOLDER\pack\*" "$env:APPVEYOR_BUILD_FOLDER-$env:CC_SHORTNAME-static\*"
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
            -DCMAKE_BUILD_TYPE=RelWithDebInfo `
            -DCMAKE_INSTALL_PREFIX="$env:APPVEYOR_BUILD_FOLDER-$env:CC_SHORTNAME-dynamic" `
            -DUA_BUILD_EXAMPLES:BOOL=ON `
            -DUA_ENABLE_AMALGAMATION:BOOL=OFF ..
    & cmake --build . --target install --config RelWithDebInfo
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0)
    {
        Write-Host -ForegroundColor Red "`n`n*** Make install failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    cd ..
    & 7z a -tzip open62541-$env:CC_SHORTNAME-dynamic.zip "$env:APPVEYOR_BUILD_FOLDER\pack\*" "$env:APPVEYOR_BUILD_FOLDER-$env:CC_SHORTNAME-static\*"
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0)
    {
        Write-Host -ForegroundColor Red "`n`n*** Zipping failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
    Remove-Item -Path build -Recurse -Force

    # Only execute unit tests on vs2017 to save compilation time
    if ($env:CC_SHORTNAME -eq "vs2017") {
        Write-Host -ForegroundColor Green "`n###################################################################"
        Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with unit tests #####`n"
        New-Item -ItemType directory -Path "build"
        cd build
        & cmake $cmake_cnf `
                -DCHECK_PREFIX=c:\check `
                -DCMAKE_BUILD_TYPE=Debug `
                -DUA_BUILD_EXAMPLES=OFF `
                -DUA_BUILD_UNIT_TESTS=ON `
                -DUA_ENABLE_DA=ON `
                -DUA_ENABLE_DISCOVERY=ON `
                -DUA_ENABLE_DISCOVERY_MULTICAST=ON `
                -DUA_ENABLE_ENCRYPTION:BOOL=$build_encryption `
                -DUA_ENABLE_JSON_ENCODING:BOOL=ON `
                -DUA_ENABLE_PUBSUB:BOOL=ON `
                -DUA_ENABLE_PUBSUB_DELTAFRAMES:BOOL=ON `
                -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON `
                -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON ..
        & cmake --build . --config Debug
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
        & cmake --build . --target test-verbose --config debug
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
    }

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
