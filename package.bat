@echo off
set "ZIP_NAME=Meo3_Arduino.zip"

echo ---------------------------------------------------
echo  Packaging ONLY Arduino Framework files...
echo ---------------------------------------------------

:: 1. Remove previous zip
if exist "%ZIP_NAME%" del "%ZIP_NAME%"

:: 2. Run PowerShell to zip ONLY the specific allow-listed files/folders
:: We look for: src, examples, library.json, library.properties, keywords.txt, README*, LICENSE*
echo Adding: src, examples, library.json, README...

powershell -Command "Get-ChildItem | Where-Object { $_.Name -in @('src', 'examples', 'library.json', 'library.properties', 'keywords.txt') -or $_.Name -like 'README*' -or $_.Name -like 'LICENSE*' } | Compress-Archive -DestinationPath '%ZIP_NAME%' -Force"

:: 3. Verify
if exist "%ZIP_NAME%" (
    echo.
    echo  [SUCCESS] Created clean package: %ZIP_NAME%
    echo  Contains only source code and metadata.
    echo.
) else (
    echo.
    echo  [ERROR] Failed to create zip.
    echo.
)

pause