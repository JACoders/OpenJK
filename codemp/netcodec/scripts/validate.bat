@echo off
rem Validation of netcodec on Windows (Visual Studio): configure, build and test the MSVC presets (x64 Release and
rem Debug, Win32 Release, x64 with AddressSanitizer), then the size benchmark, the codec benchmark (two runs) and the
rem Win32 trainer, which must rewrite the committed tables unchanged. Needs the traces of
rem docs_rewrite\netcode\data\traces (export_rans_trace.py, export_codec_trace.py, export_demo_traces.py).
rem Run from anywhere (cmd or PowerShell): codemp\netcodec\scripts\validate.bat
rem Output: the console and build-netcodec\validate-windows.log; the summary and the benchmark figures come last.
setlocal EnableDelayedExpansion
cd /d "%~dp0.."
set "B=..\..\build-netcodec"
set "T=..\..\docs_rewrite\netcode\data\traces"
if not exist "%B%" mkdir "%B%"
set "LOG=%B%\validate-windows.log"
set "OUT=%TEMP%\netcodec-validate"
if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%"
set FAILED=0
set "SUMMARY=%OUT%\summary.txt"
type nul > "%SUMMARY%"
type nul > "%LOG%"

call :step "msvc-x64: configure" cmake --preset msvc-x64
call :step "msvc-x64-release: build" cmake --build --preset msvc-x64-release --parallel
call :step "msvc-x64-release: tests" ctest --preset msvc-x64-release
call :step "msvc-x64-debug: build" cmake --build --preset msvc-x64-debug --parallel
call :step "msvc-x64-debug: tests" ctest --preset msvc-x64-debug
call :step "msvc-x86: configure" cmake --preset msvc-x86
call :step "msvc-x86-release: build" cmake --build --preset msvc-x86-release --parallel
call :step "msvc-x86-release: tests" ctest --preset msvc-x86-release
call :step "msvc-asan: configure" cmake --preset msvc-asan
call :step "msvc-asan: build" cmake --build --preset msvc-asan --parallel
call :step "msvc-asan: tests" ctest --preset msvc-asan

call :bench "size benchmark, msvc-x64" size "%B%\msvc-x64\bench\Release\netcodec_bench_size.exe" "%T%\holdout.bin"
call :bench "size benchmark, msvc-asan" size-asan "%B%\msvc-asan\bench\RelWithDebInfo\netcodec_bench_size.exe" "%T%\holdout.bin"
call :bench "codec benchmark, msvc-x64, run 1" codec1 "%B%\msvc-x64\bench\Release\netcodec_bench_codec.exe" "%T%\codec_trace.bin" 10
call :bench "codec benchmark, msvc-x64, run 2" codec2 "%B%\msvc-x64\bench\Release\netcodec_bench_codec.exe" "%T%\codec_trace.bin" 10
call :step "trainer msvc-x86: train" "%B%\msvc-x86\tools\Release\netcodec_train.exe" "%T%\train.bin" "%OUT%\tables.inc"
call :step "trainer msvc-x86 writes the committed tables" fc.exe /b "%OUT%\tables.inc" src\generated\jka_tables.inc

call :out ""
call :out "=== results"
for %%f in (size size-asan codec1 codec2) do (
	call :out "--- %%f"
	type "%OUT%\%%f.txt" 2>nul
	type "%OUT%\%%f.txt" >> "%LOG%" 2>nul
)
call :out ""
call :out "=== summary"
type "%SUMMARY%"
type "%SUMMARY%" >> "%LOG%"
if %FAILED%==0 (call :out "ALL OK") else (call :out "SOME STEPS FAILED")
echo log: %LOG%
rmdir /s /q "%OUT%"
exit /b %FAILED%

rem ---- one step: label, then the command; its output goes to the console and the log
:step
set "NAME=%~1"
shift
set "CMD="
:collect
if "%~1"=="" goto run
set CMD=!CMD! %1
shift
goto collect
:run
call :out ""
call :out "=== %NAME%"
call %CMD% > "%OUT%\step.txt" 2>&1
set RC=!ERRORLEVEL!
type "%OUT%\step.txt"
type "%OUT%\step.txt" >> "%LOG%"
if !RC!==0 (>> "%SUMMARY%" echo ok    !NAME!) else (>> "%SUMMARY%" echo FAIL  !NAME!& set FAILED=1)
exit /b 0

rem ---- one benchmark: label, output name, then the command; its output is kept for the results
:bench
set "NAME=%~1"
set "FILE=%~2"
shift
shift
set "CMD="
:collectb
if "%~1"=="" goto runb
set CMD=!CMD! %1
shift
goto collectb
:runb
call :out ""
call :out "=== %NAME%"
call %CMD% > "%OUT%\%FILE%.txt" 2>&1
set RC=!ERRORLEVEL!
type "%OUT%\%FILE%.txt" >> "%LOG%"
if !RC!==0 (>> "%SUMMARY%" echo ok    !NAME!) else (>> "%SUMMARY%" echo FAIL  !NAME!& set FAILED=1)
exit /b 0

rem ---- a line to the console and the log
:out
if "%~1"=="" (echo.& >> "%LOG%" echo.) else (echo %~1& >> "%LOG%" echo %~1)
exit /b 0
