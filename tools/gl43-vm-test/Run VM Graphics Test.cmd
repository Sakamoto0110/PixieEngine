@echo off
setlocal
pushd "%~dp0" || exit /b 1
if not exist "Results" mkdir "Results"
echo Running the exported Light Lab player for five seconds...
"%CD%\Player\pixie-player.exe" --quit-after 5 --screenshot "%CD%\Results\player.png" >"Results\player.log" 2>&1
set "PLAYER_EXIT=%ERRORLEVEL%"
type "Results\player.log"
echo Player exit code: %PLAYER_EXIT%
echo.
echo Opening the same scene in the Editor. Try Play and Stop, then close the Editor.
"%CD%\Editor\PixieEditor.exe" --sdk "%CD%\SDK" --sdk-profile authoring --native-configuration Release --project "%CD%\Scene" --recent off >"Results\editor.log" 2>&1
set "EDITOR_EXIT=%ERRORLEVEL%"
type "Results\editor.log"
echo Editor exit code: %EDITOR_EXIT%
echo.
echo Results are in "%CD%\Results".
pause
popd
if not "%PLAYER_EXIT%"=="0" exit /b %PLAYER_EXIT%
exit /b %EDITOR_EXIT%
