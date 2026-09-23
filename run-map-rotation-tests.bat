@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run-map-rotation-tests.ps1" %*
set "EXIT_CODE=%ERRORLEVEL%"
echo.
if not "%EXIT_CODE%"=="0" echo There are failed map rotation tests. See map-rotation-test-results.
pause
exit /b %EXIT_CODE%
