@echo off
title Auto Decoder Pro - Portable
color 0A

echo ========================================
echo    Auto Decoder Pro - Portable Version
echo ========================================
echo.

set AUTO_DECODER_PORTABLE=1
set AUTO_DECODER_DATA=.\data

if not exist ".\data" mkdir data
if not exist ".\data\cache" mkdir data\cache
if not exist ".\data\logs" mkdir data\logs

start "" "auto_decoder_pro.exe"

echo Application started in portable mode!
echo Data folder: .\data
echo.
pause