# cmake configuration command line
$cmake_args = @('-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake',
                '-DVCPKG_TARGET_TRIPLET=x64-windows-static',
                '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<IF:$<CONFIG:Debug>,Debug,>',
                '-G', $env:GENERATOR,
                '-A', $env:ARCH
               )

Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with unit tests #####`n"
New-Item -ItemType directory -Path "build"
cd build
cmake @cmake_args `
      -DBUILD_SHARED_LIBS:BOOL=OFF `
      -DCMAKE_BUILD_TYPE=Debug `
      -DUA_BUILD_EXAMPLES=OFF `
      -DUA_BUILD_UNIT_TESTS=ON `
      -DUA_ENABLE_DA=ON `
      -DUA_ENABLE_DISCOVERY=ON `
      -DUA_ENABLE_ENCRYPTION:STRING=MBEDTLS `
      -DUA_ENABLE_JSON_ENCODING:BOOL=ON `
      -DUA_ENABLE_PUBSUB:BOOL=ON `
      -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON `
      -DUA_FORCE_WERROR=ON `
      ..
cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}

Write-Host -ForegroundColor Green "`n### Running unit tests ###`n"
cmake --build . --target test-verbose -j 1
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Test failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n##### Building $env:CC_NAME examples #####`n"
New-Item -ItemType directory -Path "build"
cd build
cmake @cmake_args `
      -DBUILD_SHARED_LIBS:BOOL=OFF `
      -DCMAKE_BUILD_TYPE=Debug `
      -DUA_BUILD_EXAMPLES:BOOL=ON `
      -DUA_FORCE_WERROR=ON `
      ..
cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make install failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with full NS0 #####`n"
New-Item -ItemType directory -Path "build"
cd build
cmake @cmake_args `
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
cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force

Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME (.dll) #####`n"
New-Item -ItemType directory -Path "build"
cd build
cmake @cmake_args `
      -DBUILD_SHARED_LIBS:BOOL=ON `
      -DCMAKE_BUILD_TYPE=Debug `
      -DUA_FORCE_WERROR=ON `
      ..
cmake --build .
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse -Force
