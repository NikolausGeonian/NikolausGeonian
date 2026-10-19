@echo off
setlocal
cd /d "%~dp0"
where gcc >nul 2>nul
if errorlevel 1 (
  echo Please install MinGW-w64, and add its bin directory to PATH.
  exit /b 1
)
if not exist build mkdir build
if not exist dist mkdir dist
windres -i app.rc -o build\app.o
if errorlevel 1 exit /b 1
gcc -std=c11 -O2 -Wall -Wextra -Werror -finput-charset=UTF-8 -municode -mwindows -static main.c pressure_calc.c build\app.o -o dist\Pressure25.exe -lcomctl32 -lgdi32 -luser32 -lm -Wl,--dynamicbase,--nxcompat
if errorlevel 1 exit /b 1
gcc -std=c11 -O2 -Wall -Wextra -Werror -finput-charset=UTF-8 -static pressure_calc.c test_calc.c -o build\test_calc.exe -lm
if errorlevel 1 exit /b 1
gcc -std=c11 -O2 -Wall -Wextra -Werror -finput-charset=UTF-8 -municode -mconsole -static -DGUI_TEST main.c pressure_calc.c build\app.o -o build\test_gui.exe -lcomctl32 -lgdi32 -luser32 -lm
if errorlevel 1 exit /b 1
build\test_calc.exe
if errorlevel 1 exit /b 1
build\test_gui.exe
if errorlevel 1 exit /b 1
echo Built: dist\Pressure25.exe
