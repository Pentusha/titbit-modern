@echo off
wsl.exe -d Ubuntu-24.04 --cd "%~dp0." -- sh ./start-game.sh %*
if errorlevel 1 pause
