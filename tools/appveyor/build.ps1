$ErrorActionPreference = "Stop"

cd $env:APPVEYOR_BUILD_FOLDER

$make_cmd = "& $env:MAKE"

# Collect files for .zip packing
New-Item -ItemType directory -Path pack
Copy-Item LICENSE pack
Copy-Item AUTHORS pack
Copy-Item README.md pack

Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Building Documentation on $env:CC_NAME #####`n"
New-Item -ItemType directory -Path build
cd build
& cmake -DMIKTEX_BINARY_PATH=c:\miktex\texmfs\install\miktex\bin -DCMAKE_BUILD_TYPE=Release -DUA_COMPILE_AS_CXX:BOOL=$env:FORCE_CXX -DUA_BUILD_EXAMPLES:BOOL=OFF -G"$env:CC_NAME" ..
& cmake --build . --target doc_latex
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
	Write-Host -ForegroundColor Red "`n`n*** Make doc_latex. Exiting ... ***"
	exit $LASTEXITCODE
}
& cmake --build . --target doc_pdf
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
	Write-Host -ForegroundColor Red "`n`n*** Make doc_pdf. Exiting ... ***"
	exit $LASTEXITCODE
}
cd ..
Move-Item -Path "build\doc_latex\open62541.pdf" -Destination pack\
Remove-Item -Path build -Recurse


Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake -DUA_BUILD_EXAMPLES:BOOL=ON -DUA_COMPILE_AS_CXX:BOOL=$env:FORCE_CXX -G"$env:CC_NAME" ..
Invoke-Expression $make_cmd
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
	Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
	exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse


Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with full NS0 #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake -DUA_BUILD_EXAMPLES:BOOL=ON -DUA_ENABLE_FULL_NS0:BOOL=ON -DUA_COMPILE_AS_CXX:BOOL=$env:FORCE_CXX -G"$env:CC_NAME" ..
Invoke-Expression $make_cmd
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
	Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
	exit $LASTEXITCODE
}
cd ..
Remove-Item -Path build -Recurse


Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with amalgamation #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_BUILD_EXAMPLES:BOOL=ON -DUA_ENABLE_AMALGAMATION:BOOL=ON -DUA_COMPILE_AS_CXX:BOOL=$env:FORCE_CXX -DBUILD_SHARED_LIBS:BOOL=OFF -G"$env:CC_NAME" ..
Invoke-Expression $make_cmd
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
	Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
	exit $LASTEXITCODE
}
cd ..
New-Item -ItemType directory -Path pack_tmp
Move-Item -Path "build\open62541.c" -Destination pack_tmp\
Move-Item -Path "build\open62541.h" -Destination pack_tmp\
Move-Item -Path "build\$env:OUT_DIR_EXAMPLES\server.exe" -Destination pack_tmp\
Move-Item -Path "build\$env:OUT_DIR_EXAMPLES\client.exe" -Destination pack_tmp\
if ($env:CC_SHORTNAME -eq "mingw") {
	Move-Item -Path "build\$env:OUT_DIR_LIB\libopen62541.a" -Destination pack_tmp\
} else {
	Move-Item -Path "build\$env:OUT_DIR_LIB\open62541.lib" -Destination pack_tmp\
}
& 7z a -tzip open62541-$env:CC_SHORTNAME-static.zip "$env:APPVEYOR_BUILD_FOLDER\pack\*" "$env:APPVEYOR_BUILD_FOLDER\pack_tmp\*"
Remove-Item -Path pack_tmp -Recurse
Remove-Item -Path build -Recurse


Write-Host -ForegroundColor Green "`n###################################################################"
Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with amalgamation and .dll #####`n"
New-Item -ItemType directory -Path "build"
cd build
& cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_BUILD_EXAMPLES:BOOL=ON -DUA_ENABLE_AMALGAMATION:BOOL=ON -DUA_COMPILE_AS_CXX:BOOL=$env:FORCE_CXX -DBUILD_SHARED_LIBS:BOOL=ON -G"$env:CC_NAME" ..
Invoke-Expression $make_cmd
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
	Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
	exit $LASTEXITCODE
}
cd ..
New-Item -ItemType directory -Path pack_tmp
Move-Item -Path "build\open62541.c" -Destination pack_tmp\
Move-Item -Path "build\open62541.h" -Destination pack_tmp\
Move-Item -Path "build\$env:OUT_DIR_EXAMPLES\server.exe" -Destination pack_tmp\
Move-Item -Path "build\$env:OUT_DIR_EXAMPLES\client.exe" -Destination pack_tmp\
if ($env:CC_SHORTNAME -eq "mingw") {
	Move-Item -Path "build\$env:OUT_DIR_LIB\libopen62541.dll" -Destination pack_tmp\
	Move-Item -Path "build\$env:OUT_DIR_LIB\libopen62541.dll.a" -Destination pack_tmp\
} else {
	Move-Item -Path "build\$env:OUT_DIR_LIB\open62541.dll" -Destination pack_tmp\
	Move-Item -Path "build\$env:OUT_DIR_LIB\open62541.pdb" -Destination pack_tmp\
}
& 7z a -tzip open62541-$env:CC_SHORTNAME-dynamic.zip "$env:APPVEYOR_BUILD_FOLDER\pack\*" "$env:APPVEYOR_BUILD_FOLDER\pack_tmp\*"
Remove-Item -Path pack_tmp -Recurse
Remove-Item -Path build -Recurse

if ($env:CC_SHORTNAME -eq "vs2015") {
	# Only execute unit tests on vs2015 to save compilation time
	New-Item -ItemType directory -Path "build"
	cd build
	Write-Host -ForegroundColor Green "`n###################################################################"
	Write-Host -ForegroundColor Green "`n##### Testing $env:CC_NAME with unit tests #####`n"
	& cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=OFF -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON  -DCMAKE_LIBRARY_PATH=c:\check\lib -DCMAKE_INCLUDE_PATH=c:\check\include -DUA_COMPILE_AS_CXX:BOOL=$env:FORCE_CXX -G"$env:CC_NAME" ..
	Invoke-Expression $make_cmd
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    	Write-Host -ForegroundColor Red "`n`n*** Make failed. Exiting ... ***"
    	exit $LASTEXITCODE
    }
	& cmake --build . --target test-verbose --config debug
}


# do not cache log
Remove-Item -Path c:\miktex\texmfs\data\miktex\log -Recurse