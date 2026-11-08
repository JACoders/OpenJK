@echo off
rem A/B (owner, Windows VM): bench_codec of a reference against the working tree, MSVC x64 Release, run alternately
rem on logical CPU 1 with high priority; prints each run, then the best of each build against ref with the checks of
rem ab_summary.ps1 (netcodec mismatches, a busy machine, code placement).
rem The reference: ABM_REF_DIR=<a copy holding codemp\netcodec> if set; else extracted here when git is in the PATH
rem (ABM_REF=<commit>, default HEAD); otherwise the copy that ab_msvc_ref.sh [commit] writes on the host into
rem build-netcodec\ab-msvc, used only if its commit is HEAD (read from .git, no git needed): a stale copy stops the run
rem (ABM_ANY_REF=1 compares it anyway).
rem ABM_ARCH=Win32: 32-bit builds (default x64).
rem Usage (cmd or PowerShell, from anywhere): codemp\netcodec\scripts\ab_msvc.bat [rounds]
setlocal EnableDelayedExpansion
set "R=%~dp0..\..\.."
for %%i in ("%R%") do set "R=%%~fi"
set "ROUNDS=%~1"
if "%ROUNDS%"=="" set "ROUNDS=6"
if not defined ABM_ARCH set "ABM_ARCH=x64"
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

if defined ABM_REF_DIR (
	set "REFDIR=%ABM_REF_DIR%\codemp\netcodec"
	echo ref = the copy in %ABM_REF_DIR%; cur = the working tree
	goto refready
)
where git >nul 2>&1
if not errorlevel 1 (
	mkdir "%W%\ref"
	if not defined ABM_REF set "ABM_REF=HEAD"
	git -c safe.directory=* -C "%R%" archive !ABM_REF! codemp/netcodec | "%SystemRoot%\System32\tar.exe" -x -f - -C "%W%\ref"
	if not exist "%W%\ref\codemp\netcodec\CMakeLists.txt" (
		echo git archive of !ABM_REF! failed
		exit /b 1
	)
	set "REFDIR=%W%\ref\codemp\netcodec"
	echo ref = !ABM_REF! ^(HEAD is !HEADC:~0,9!, extracted with git^); cur = the working tree
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

:refready
echo building the reference and the working tree (%ABM_ARCH%) ...
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
cmake -S "%~1" -B "%~2" -G "Visual Studio 18 2026" -A %ABM_ARCH% -Dnetcodec_OPENJK_DIR="%R%" -Dnetcodec_TRACE_DIR="%R%\docs_rewrite\netcode\data\traces" > "%W%\cfg.txt" 2>&1
if errorlevel 1 (type "%W%\cfg.txt" & exit /b 1)
cmake --build "%~2" --config Release --target netcodec_bench_codec --parallel > "%W%\bld.txt" 2>&1
if errorlevel 1 (type "%W%\bld.txt" & exit /b 1)
exit /b 0
