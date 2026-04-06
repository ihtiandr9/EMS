@echo off
C:\BC31\BIN\BCC -IC:\BC31\INCLUDE -eemstest.exe -mh main.c emsdri.c
if errorlevel 1 exit /B 1
