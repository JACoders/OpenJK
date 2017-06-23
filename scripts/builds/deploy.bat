setlocal
set CYGWIN_ROOT=C:\cygwin
set WIN32_ROOT=/cygdrive/c
set DEPLOY_LOCATION=ojkwinbuilder@upload.openjk.org:/home/ojkwinbuilder/builds/
set ZIP_FILE=%1

if "%APPVEYOR_FORCED_BUILD%"=="True" (
	%CYGWIN_ROOT%\setup-x86.exe -qnNdO -R %CYGWIN_ROOT% -s http://cygwin.mirror.constant.com -l %CYGWIN_ROOT%/var/cache/setup -P rsync
	%CYGWIN_ROOT%\bin\bash -lc 'rsync -avz --progress -e "ssh -p 29022 -o StrictHostKeyChecking=no -i %WIN32_ROOT%/users/appveyor/.ssh/id_rsa -p --chmod=0644" --protocol=29 %WIN32_ROOT%/projects/openjk/%ZIP_FILE% "%DEPLOY_LOCATION%"'
)
