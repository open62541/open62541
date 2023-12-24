try {

    Write-Host -ForegroundColor Green "`n### Installing sphinx ###`n"
    & choco install -y sphinx --no-progress --source python
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Install failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

    if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
        Write-Host -ForegroundColor Green "`n### Installing msys64 ###`n"
        & choco install -y msys2 --no-progress --params="/InstallDir:$env:MSYS2_ROOT /NoPath"
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Install failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
        # pacman may complain that the directory does not exist, thus create it.
        # See https://github.com/open62541/open62541/issues/2068
        & C:\msys64\usr\bin\mkdir -p /var/cache/pacman/pkg
        # Update all packages. Ensure that we have up-to-date clang version.
        # Otherwise we run into issue: https://github.com/msys2/MINGW-packages/issues/6576
        & C:\msys64\usr\bin\pacman -Syu

    Write-Host -ForegroundColor Green "`n### Installing mbedtls via PacMan ###`n"
    & C:\msys64\usr\bin\pacman --noconfirm --needed -S mingw-w64-x86_64-mbedtls
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Install of mbedTLS failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

    & C:\msys64\usr\bin\pacman --noconfirm --needed -S mingw-w64-x86_64-check
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Install of check failed. Exiting ... ***"
        exit $LASTEXITCODE
    }
} else {
    Write-Host -ForegroundColor Green "`n### Installing mbedtls via vcpkg ###`n"
    & vcpkg install mbedtls:x64-windows-static
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Install failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

    Write-Host -ForegroundColor Green "`n### Installing libcheck via vcpkg ###`n"
    & vcpkg install check:x64-windows-static
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Install failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

        Write-Host -ForegroundColor Green "`n### Installing DrMemory ###`n"
        & choco install -y --no-progress drmemory.portable
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Install failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
        #$env:Path = 'C:\Program Files (x86)\Dr. Memory\bin;' + $env:Path
        #[System.Environment]::SetEnvironmentVariable('Path', $path, 'Machine')
    }

    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Installing dependencies failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

} catch {
    # Print a detailed error message
    $FullException = ($_.Exception|format-list -force) | Out-String
    Write-Host -ForegroundColor Red "`n------------------ Exception ------------------`n$FullException`n"
    [Console]::Out.Flush()
    # Wait a bit to make sure appveyor shows the error message
    Start-Sleep 10
    throw
}
