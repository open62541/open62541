if ($env:CC_SHORTNAME -eq "mingw" -or $env:CC_SHORTNAME -eq "clang-mingw") {
    # Check if MSYS2 is already available (e.g., in GitHub Actions)
    if (Test-Path "C:\msys64\usr\bin\pacman.exe") {
        Write-Host -ForegroundColor Green "`n### MSYS2 already available, checking for required packages ###`n"
        
        # Check if mbedtls is installed and get version info
        Write-Host -ForegroundColor Green "`n### Checking mbedtls installation ###`n"
        $mbedtls_version = & C:\msys64\usr\bin\pacman -Qi mingw-w64-x86_64-mbedtls 2>$null | Select-String "Version"
        if ($mbedtls_version) {
            Write-Host -ForegroundColor Yellow "Current mbedtls version: $mbedtls_version"
            # Check if it's a problematic version (3.5.x or newer)
            if ($mbedtls_version -match "3\.[5-9]\.|[4-9]\.") {
                Write-Host -ForegroundColor Yellow "`n### Problematic mbedtls version detected, downgrading ###`n"
                & C:\msys64\usr\bin\pacman --noconfirm -R mingw-w64-x86_64-mbedtls 2>$null
            }
        }
        
        # Install mbedtls - try current version first, if it fails with PSA errors we'll handle it in build
        Write-Host -ForegroundColor Green "`n### Installing mbedtls via PacMan ###`n"
        & C:\msys64\usr\bin\pacman --noconfirm --needed -S mingw-w64-x86_64-mbedtls
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Install of mbedTLS failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
        
        # Check if check is installed
        $check_check = & C:\msys64\usr\bin\pacman -Qq mingw-w64-x86_64-check 2>$null
        if (-not $check_check) {
            Write-Host -ForegroundColor Green "`n### Installing check via PacMan ###`n"
            & C:\msys64\usr\bin\pacman --noconfirm --needed -S mingw-w64-x86_64-check
            if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
                Write-Host -ForegroundColor Red "`n`n*** Install of check failed. Exiting ... ***"
                exit $LASTEXITCODE
            }
        } else {
            Write-Host -ForegroundColor Green "`n### check already installed ###`n"
        }
    } else {
        Write-Host -ForegroundColor Green "`n### Installing msys64 ###`n"
        & choco install -y msys2 --no-progress --params="/InstallDir:$env:MSYS2_ROOT /NoPath"
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Install failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
        # pacman may complain that the directory does not exist, thus create it.
        # Se https://github.com/open62541/open62541/issues/2068
        & C:\msys64\usr\bin\mkdir -p /var/cache/pacman/pkg
        # Update all packages. Ensure that we have up-to-date clang version.
        # Otherwise we run into issue: https://github.com/msys2/MINGW-packages/issues/6576
        & C:\msys64\usr\bin\pacman -Syu

        Write-Host -ForegroundColor Green "`n### Installing specific mbedtls version via PacMan ###`n"
        # Install a specific known-good version that doesn't have PSA API conflicts  
        & C:\msys64\usr\bin\pacman --noconfirm --needed -S mingw-w64-x86_64-mbedtls=3.4.1-1
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Yellow "`n### Specific version 3.4.1 failed, trying 3.4.0 ###`n"
            & C:\msys64\usr\bin\pacman --noconfirm --needed -S mingw-w64-x86_64-mbedtls=3.4.0-1
            if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
                Write-Host -ForegroundColor Red "`n`n*** Install of stable mbedTLS failed. Exiting ... ***"
                exit $LASTEXITCODE
            }
        }

        & C:\msys64\usr\bin\pacman --noconfirm --needed -S mingw-w64-x86_64-check
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            Write-Host -ForegroundColor Red "`n`n*** Install of check failed. Exiting ... ***"
            exit $LASTEXITCODE
        }
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
}
