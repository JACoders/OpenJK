echo "%APPVEYOR_FORCED_BUILD%"
if "%APPVEYOR_FORCED_BUILD%"=="True" (
	C:\cygwin\setup-x86.exe -qnNdO -R C:/cygwin -s http://cygwin.mirror.constant.com -l C:/cygwin/var/cache/setup -P rsync
	rsync -avz --progress -e 'ssh -p 29022 ' --protocol=29 openjk-windows*.zip ojkwinbuilder@upload.openjk.org:/home/ojkwinbuilder/builds/
)
