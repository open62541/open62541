$ErrorActionPreference = "Stop"

try {

    Write-Host -ForegroundColor Green "`n### Installing sphinx ###`n"
    & choco install -y sphinx --no-progress --source python

    if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
        Write-Host -ForegroundColor Green "`n### Installing msys64 ###`n"
        & choco install -y msys2 --no-progress --params="/InstallDir:$env:MSYS2_ROOT /NoUpdate /NoPath"

        Write-Host -ForegroundColor Green "`n### Installing mbedtls via PacMan ###`n"
        # pacman may complain that the directory does not exist, thus create it.
        # See https://github.com/open62541/open62541/issues/2068
        & C:\msys64\usr\bin\mkdir -p /var/cache/pacman/pkg
        & C:\msys64\usr\bin\pacman --noconfirm -S mingw-w64-x86_64-mbedtls

        Write-Host -ForegroundColor Green "`n### Installing clang via PacMan ###`n"
        & C:\msys64\usr\bin\pacman --noconfirm -S mingw-w64-x86_64-clang mingw-w64-i686-clang
    } elseif ($env:CC_SHORTNAME -eq "vs2015" -or $env:CC_SHORTNAME -eq "vs2017") {
        Write-Host -ForegroundColor Green "`n### Installing mbedtls via vcpkg ###`n"
        & vcpkg install mbedtls:x86-windows-static

        Write-Host -ForegroundColor Green "`n### Installing libcheck via vcpkg ###`n"
        & vcpkg install check:x86-windows-static

        Write-Host -ForegroundColor Green "`n### Installing DrMemory ###`n"
        & choco install -y --no-progress drmemory.portable
        $env:Path = 'C:\Program Files (x86)\Dr. Memory\bin;' + $env:Path        
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
