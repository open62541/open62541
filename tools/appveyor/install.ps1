$ErrorActionPreference = "Stop"

try {
    & git submodule --quiet update --init --recursive

    Write-Host -ForegroundColor Green "`n### Installing CMake and python ###`n"
    & cinst --no-progress cmake python2
    & C:\Python27\python.exe -m pip install --upgrade pip
    & C:\Python27\Scripts\pip.exe install six

    Write-Host -ForegroundColor Green "`n### Installing sphinx ###`n"
    & C:\Python27\Scripts\pip.exe install --user sphinx sphinx_rtd_theme

    Write-Host -ForegroundColor Green "`n### Installing Miktex ###`n"
    if (-not (Test-Path "c:\miktex\texmfs\install\miktex\bin\pdflatex.exe")) {
        & appveyor DownloadFile https://ftp.uni-erlangen.de/mirrors/CTAN/systems/win32/miktex/setup/windows-x86/miktex-portable-2.9.6753.exe
        & 7z x miktex-portable-2.9.6753.exe -oc:\miktex -bso0 -bsp0

        # Remove some big files to reduce size to be cached
        Remove-Item -Path c:\miktex\texmfs\install\doc -Recurse
        Remove-Item -Path c:\miktex\texmfs\install\miktex\bin\biber.exe
        Remove-Item -Path c:\miktex\texmfs\install\miktex\bin\a5toa4.exe
    }

    Write-Host -ForegroundColor Green "`n### Installing graphviz ###`n"
    & cinst --no-progress graphviz
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Installing graphviz failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

    Write-Host -ForegroundColor Green "`n### Installing mbedtls ###`n"

    if ($env:CC_SHORTNAME -eq "mingw") {
        & C:\msys64\usr\bin\pacman --noconfirm -S mingw-w64-x86_64-mbedtls
    } elseif ($env:CC_SHORTNAME -eq "vs2015") {
        # we need the static version, since open62541 is built with /MT
        # vcpkg currently only supports VS2015 and newer builds
        & vcpkg install mbedtls:x86-windows-static
    }
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n*** Installing mbedtls failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

    if ($env:CC_SHORTNAME -eq "vs2015") {
        Write-Host -ForegroundColor Green "`n### Installing libcheck ###`n"
        & appveyor DownloadFile https://github.com/Pro/check/releases/download/0.12.0_win/check.zip
        & 7z x check.zip -oc:\ -bso0 -bsp0

        Write-Host -ForegroundColor Green "`n### Installing DrMemory ###`n"
        & cinst --no-progress drmemory.portable
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
