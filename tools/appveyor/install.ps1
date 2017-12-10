$ErrorActionPreference = "Stop"

& git submodule --quiet update --init --recursive


Write-Host -ForegroundColor Green "`n### Installing CMake and python ###`n"
& cinst --no-progress cmake python2

Write-Host -ForegroundColor Green "`n### Installing sphinx ###`n"
& pip install --user sphinx sphinx_rtd_theme

Write-Host -ForegroundColor Green "`n### Installing Miktex ###`n"
if (-not (Test-Path "c:\miktex\texmfs\install\miktex\bin\pdflatex.exe")) {
	& appveyor DownloadFile https://ftp.uni-erlangen.de/mirrors/CTAN/systems/win32/miktex/setup/miktex-portable.exe
	& 7z x miktex-portable.exe -oc:\miktex -bso0 -bsp0

	# Remove some big files to reduce size to be cached
	Remove-Item -Path c:\miktex\texmfs\install\doc -Recurse
	Remove-Item -Path c:\miktex\texmfs\install\miktex\bin\biber.exe
	Remove-Item -Path c:\miktex\texmfs\install\miktex\bin\icudt58.dll
	Remove-Item -Path c:\miktex\texmfs\install\miktex\bin\a5toa4.exe
}

Write-Host -ForegroundColor Green "`n### Installing graphviz ###`n"
& cinst --no-progress graphviz.portable


if ($env:CC_SHORTNAME -eq "vs2015") {
	Write-Host -ForegroundColor Green "`n### Installing libcheck ###`n"
	& appveyor DownloadFile https://github.com/Pro/check/releases/download/0.12.0_win/check.zip
	& 7z x check.zip -oc:\ -bso0 -bsp0

	Write-Host -ForegroundColor Green "`n### Installing DrMemory ###`n"
	& cinst --no-progress drmemory.portable
}