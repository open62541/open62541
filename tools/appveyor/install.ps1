$ErrorActionPreference = "Continue"

try {
    & git submodule sync
    & git submodule update --init --recursive

    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Host -ForegroundColor Red "`n`n***Cloning submodules failed. Exiting ... ***"
        exit $LASTEXITCODE
    }

    Write-Host -ForegroundColor Green "`n### Installing sphinx ###`n"
    & cinst sphinx --source python

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
