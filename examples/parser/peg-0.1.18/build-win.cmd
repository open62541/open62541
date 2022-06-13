@echo off
call "%VS100COMNTOOLS%vsvars32.bat"
msbuild peg.sln /p:Configuration=Release

xcopy /Y /D Release\*.exe .\
