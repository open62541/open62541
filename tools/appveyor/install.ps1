$ErrorActionPreference = "Stop"

try {
    & git submodule sync
    & git submodule --quiet update --init --recursive

    Write-Host -ForegroundColor Green "`n### CMake and Python pre-installed AppVeyor Windows build VMs ###`n"

    Write-Host -ForegroundColor Green "`n### Installing sphinx ###`n"
    & cinst sphinx --source python

    Write-Host -ForegroundColor Green "`n### Installing mbedtls ###`n"

    if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
        # pacman may complain that the directory does not exist, thus create it.
        # See https://github.com/open62541/open62541/issues/2068
        & C:\msys64\usr\bin\mkdir -p /var/cache/pacman/pkg
        & C:\msys64\usr\bin\pacman --noconfirm -S mingw-w64-x86_64-mbedtls
    } elseif ($env:CC_SHORTNAME -eq "vs2015" -or $env:CC_SHORTNAME -eq "vs2017") {
        # we need the static version, since open62541 is built with /MT
        # vcpkg currently only supports VS2015 and newer builds
        & vcpkg install mbedtls:x86-windows-static
    }
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Installing mbedtls failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

    if ($env:CC_SHORTNAME -eq "vs2015" -or $env:CC_SHORTNAME -eq "vs2017") {
        Write-Host -ForegroundColor Green "`n### Installing libcheck ###`n"
        & appveyor DownloadFile https://github.com/Pro/check/releases/download/0.12.0_dbg/check.zip
        & 7z x check.zip -oc:\ -bso0 -bsp0

        Write-Host -ForegroundColor Green "`n### Installing DrMemory ###`n"
        & cinst --no-progress drmemory.portable
        $env:Path = 'C:\Program Files (x86)\Dr. Memory\bin;' + $env:Path        
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
