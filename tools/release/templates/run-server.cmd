@echo off
setlocal
cd /d "%~dp0"
if /i "%~1"=="--private-console" goto launch
rem Use a separate interactive console: Ctrl-C must reach only this server family.
start "EQEmu RoF2 server" /wait cmd /c ""%~f0" --private-console"
exit /b %errorlevel%
:launch
if not defined EQEMU_DATABASE_NAME set EQEMU_DATABASE_NAME=peq
if not defined EQEMU_ZONE_COUNT set EQEMU_ZONE_COUNT=6
py -3 supervise-server.py --server-directory "%CD%" --expected-database "%EQEMU_DATABASE_NAME%" --recovery-executable "%CD%\bin\inventory_recovery_cli.exe" --enable-supervised-inventory --zone-count %EQEMU_ZONE_COUNT%
exit /b %errorlevel%
