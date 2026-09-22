@echo off
setlocal
call D:\vstudio\2022\community\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /W4 /WX /utf-8 /std:c11 /TC /I. /I..\..\projects\app_gatt\config test_acc_fifo.c /Febuild\test_acc_fifo.exe
if errorlevel 1 exit /b 1
build\test_acc_fifo.exe
if errorlevel 1 exit /b 1
cl /nologo /W4 /WX /utf-8 /std:c11 /TC /I. test_sensor_flash_batch.c /Febuild\test_sensor_flash_batch.exe
if errorlevel 1 exit /b 1
build\test_sensor_flash_batch.exe
if errorlevel 1 exit /b 1
cl /nologo /W4 /WX /utf-8 /std:c11 /TC /I. /I..\..\projects\app_gatt\app\api test_period_due_queue.c /Febuild\test_period_due_queue.exe
if errorlevel 1 exit /b 1
build\test_period_due_queue.exe
if errorlevel 1 exit /b 1
cl /nologo /W4 /WX /utf-8 /std:c11 /TC /I. test_ext_flash_read.c /Febuild\test_ext_flash_read.exe
if errorlevel 1 exit /b 1
build\test_ext_flash_read.exe
if errorlevel 1 exit /b 1
cl /nologo /W4 /WX /utf-8 /std:c11 /TC /I. test_as7341_fifo.c /Febuild\test_as7341_fifo.exe
if errorlevel 1 exit /b 1
build\test_as7341_fifo.exe
if errorlevel 1 exit /b 1
