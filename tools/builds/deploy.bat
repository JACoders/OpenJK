setlocal
set CYGWIN_ROOT=C:\cygwin
set DEPLOY_LOCATION=ojkwinbuilder@upload.openjk.org:/home/ojkwinbuilder/builds/

if "%APPVEYOR_FORCED_BUILD%"=="True" (
	%CYGWIN_ROOT%\setup-x86.exe -qnNdO -R %CYGWIN_ROOT% -s http://cygwin.mirror.constant.com -l %CYGWIN_ROOT%/var/cache/setup -P rsync
	%CYGWIN_ROOT%\bin\bash -lc 'rsync -avz --progress -e "ssh -p 29022" --protocol=29 openjk-windows*.zip "%DEPLOY_LOCATION%"'
)
