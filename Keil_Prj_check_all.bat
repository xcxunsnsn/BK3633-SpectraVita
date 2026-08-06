@echo off

echo clean start...
for /d /r . %%d in (Listings,obj) do (
    if exist "%%d" (             
        echo %%d | find /I "\node_modules\" > Nul && ( 
            echo skipping: %%d
        ) || (
            echo deleting: %%d
            rd /s/q "%%d"
        )
    )
)
echo clean end

set UV=C:\Keil_v512\UV4\UV4.exe

echo .>build_log.txt
echo buiding ...
rem 1st Build boot projects
echo boot building ...
for /f "delims= " %%i in ('dir /s/b boot.uvproj') do (
echo %%i
%UV% -j0 -b -C %%i -o %%i.log
type %%i.log >> build_log.txt
)

rem 2nd Build stack projects
echo stack building ...
for /f "delims= " %%i in ('dir /s/b stack.uvproj') do (
echo %%i
%UV% -j0 -b -C %%i -o %%i.log
type %%i.log >> build_log.txt
)

rem 3rd Build app lib projects
echo stack building ...
for /f "delims= " %%i in ('dir /s/b lib.uvproj') do (
echo %%i
%UV% -j0 -b -C %%i -o %%i.log
type %%i.log >> build_log.txt
)


rem Last Build app projects
echo app building ...
for /f "delims= " %%i in ('dir /s/b prj3633.uvproj') do (
echo %%i
%UV% -j0 -b -C %%i -o %%i.log
type %%i.log >> build_log.txt
)

echo build Done.

echo Find Errors...
findstr /i "Error(s)" build_log.txt
pause



