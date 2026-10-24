@echo off
setlocal
set CPP="C:/msys64/ucrt64/bin/g++.exe"
set INC=-IC:/msys64/ucrt64/include -I. -Iinclude
set LIB=-LC:/msys64/ucrt64/lib
set SRC=src\Database.cpp src\main.cpp src\models\DomainModels.cpp
%CPP% -std=c++17 %INC% %LIB% %SRC% -lsqlite3 -o lms.exe
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
echo Build successful.
call lms.exe
