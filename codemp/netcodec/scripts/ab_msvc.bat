@echo off
rem A/B (owner, Windows VM): bench_codec of git HEAD against the working tree, MSVC x64 Release, run alternately on
rem logical CPU 1 with high priority; prints each run, then the best of each build against ref with the checks of
rem ab_summary.ps1 (netcodec mismatches, a busy machine, code placement).
rem The reference: extracted here when git is in the PATH; otherwise the copy that ab_msvc_ref.sh writes on the host
rem into build-netcodec\ab-msvc, used only if its commit is HEAD (read from .git, no git needed): a stale copy stops
rem the run (ABM_ANY_REF=1 compares it anyway).
rem Usage (cmd or PowerShell, from anywhere): codemp\netcodec\scripts\ab_msvc.bat [rounds]
setlocal EnableDelayedExpansion
set "R=%~dp0..\..\.."
for %%i in ("%R%") do set "R=%%~fi"
set "ROUNDS=%~1"
if "%ROUNDS%"=="" set "ROUNDS=6"
set "W=%TEMP%\netcodec-abm"
set "T=%R%\docs_rewrite\netcode\data\traces\codec_trace.bin"
if exist "%W%" rmdir /s /q "%W%"
mkdir "%W%"

rem the commit of HEAD: .git\HEAD names a branch ("ref: refs/heads/...", in its own file or in packed-refs) or holds it
set "HEADC="
set "HEADLINE="
for /f "usebackq delims=" %%L in ("%R%\.git\HEAD") do if not defined HEADLINE set "HEADLINE=%%L"
if "!HEADLINE:~0,5!"=="ref: " (
	set "REFNAME=!HEADLINE:~5!"
	set "REFFILE=%R%\.git\!REFNAME:/=\!"
	if exist "!REFFILE!" (
		for /f "usebackq delims=" %%L in ("!REFFILE!") do if not defined HEADC set "HEADC=%%L"
	) else (
		for /f "usebackq tokens=1,2" %%a in ("%R%\.git\packed-refs") do if "%%b"=="!REFNAME!" set "HEADC=%%a"
	)
) else (
	set "HEADC=!HEADLINE!"
)
if not defined HEADC (
	echo cannot read the HEAD commit from %R%\.git
	exit /b 1
)

where git >nul 2>&1
if not errorlevel 1 (
	mkdir "%W%\ref"
	git -c safe.directory=* -C "%R%" archive HEAD codemp/netcodec | "%SystemRoot%\System32\tar.exe" -x -f - -C "%W%\ref"
	if not exist "%W%\ref\codemp\netcodec\CMakeLists.txt" (
		echo git archive of HEAD failed
		exit /b 1
	)
	set "REFDIR=%W%\ref\codemp\netcodec"
	echo ref = HEAD ^(!HEADC:~0,9!, extracted with git^); cur = the working tree
) else (
	set "REFTXT=%R%\build-netcodec\ab-msvc\ref.txt"
	set "REFC="
	set "REFDESC="
	if exist "!REFTXT!" for /f "usebackq tokens=1*" %%a in ("!REFTXT!") do if not defined REFC (
		set "REFC=%%a"
		set "REFDESC=%%b"
	)
	if not exist "%R%\build-netcodec\ab-msvc\ref\codemp\netcodec\CMakeLists.txt" set "REFC="
	if not defined REFC (
		echo no reference copy: run on the host, in Git Bash: bash codemp/netcodec/scripts/ab_msvc_ref.sh
		exit /b 1
	)
	if not "!REFC!"=="!HEADC!" if not defined ABM_ANY_REF (
		echo the reference copy is !REFC:~0,9! but HEAD is !HEADC:~0,9!: run on the host, in Git Bash:
		echo   bash codemp/netcodec/scripts/ab_msvc_ref.sh
		echo or set ABM_ANY_REF=1 to compare that older reference anyway
		exit /b 1
	)
	set "REFDIR=%R%\build-netcodec\ab-msvc\ref\codemp\netcodec"
	echo ref = !REFDESC! ^(the host's copy^); cur = the working tree
)

echo building the reference and the working tree ...
call :build "!REFDIR!" "%W%\build-ref" || exit /b 1
call :build "%R%\codemp\netcodec" "%W%\build-cur" || exit /b 1
for /l %%i in (1,1,%ROUNDS%) do (
	for %%v in (ref cur) do (
		echo %%v %%i
		start "" /b /wait /affinity 0x2 /high "%W%\build-%%v\bench\Release\netcodec_bench_codec.exe" "%T%" 10 > "%W%\out-%%v-%%i.txt"
		findstr /c:"mismatches" /c:"  encode" /c:"  decode" "%W%\out-%%v-%%i.txt"
	)
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ab_summary.ps1" "%W%"
exit /b 0

:build
cmake -S "%~1" -B "%~2" -G "Visual Studio 18 2026" -A x64 -Dnetcodec_OPENJK_DIR="%R%" -Dnetcodec_TRACE_DIR="%R%\docs_rewrite\netcode\data\traces" > "%W%\cfg.txt" 2>&1
if errorlevel 1 (type "%W%\cfg.txt" & exit /b 1)
cmake --build "%~2" --config Release --target netcodec_bench_codec --parallel > "%W%\bld.txt" 2>&1
if errorlevel 1 (type "%W%\bld.txt" & exit /b 1)
exit /b 0
