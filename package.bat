@echo off
set "FOLDER_NAME=Meo3_Arduino"
set "ZIP_NAME=Meo3_Arduino.zip"

:: 1. Cleanup
if exist "%ZIP_NAME%" del "%ZIP_NAME%"
if exist "%FOLDER_NAME%" rmdir /s /q "%FOLDER_NAME%"

:: 2. Create the Parent Folder (Required by Arduino)
mkdir "%FOLDER_NAME%"

:: 3. Copy your project files INTO that folder
xcopy "src" "%FOLDER_NAME%\src" /s /e /i /y >nul
if exist "examples" xcopy "examples" "%FOLDER_NAME%\examples" /s /e /i /y >nul
if exist "library.json" copy "library.json" "%FOLDER_NAME%\" >nul
if exist "library.properties" copy "library.properties" "%FOLDER_NAME%\" >nul
if exist "README.md" copy "README.md" "%FOLDER_NAME%\" >nul
if exist "keywords.txt" copy "keywords.txt" "%FOLDER_NAME%\" >nul

:: 4. Delete main.cpp if it snuck in (Safety)
if exist "%FOLDER_NAME%\src\main.cpp" del "%FOLDER_NAME%\src\main.cpp"

:: 5. Zip the Parent Folder
powershell -Command "Compress-Archive -Path '%FOLDER_NAME%' -DestinationPath '%ZIP_NAME%' -Force"

:: 6. Cleanup
rmdir /s /q "%FOLDER_NAME%"

echo [SUCCESS] Created %ZIP_NAME%
pause