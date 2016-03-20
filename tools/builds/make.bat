set BUILDDIR=%1
IF [%BUILDDIR%] == [] (
    set BUILDDIR=.
)
cmake -G "Visual Studio 12" -D CMAKE_INSTALL_PREFIX=install -D CMAKE_BUILD_TYPE=Release %BUILDDIR%