# Moderated LMS - Implementation Starter

This project is the implementation starter for the Moderated Learning Management System described in the CS4433 final project brief.

## Project objective

The system is a C++17 desktop application with SQLite persistence for a school LMS. It supports:

- Administrator
- Lecturer
- Head of Department
- Student

The implementation focuses on the core domain model, SQLite setup, and the role-based workflow required by the Phase 2 and Phase 3 documents.

## Build and run (Windows with MSYS2 UCRT64)

From the project root:

```bash
C:/msys64/ucrt64/bin/g++.exe -std=c++17 -I"C:/msys64/ucrt64/include" -L"C:/msys64/ucrt64/lib" src/*.cpp -lsqlite3 -o lms.exe
./lms.exe

or

cd "C:\Users\Puseletso Mokhubu\Desktop\Projects\SOFTWARE-ENGINEERING\ModeratedLMS"
.\build.bat
```

## Current implementation status

- SQLite database layer is initialized
- core domain types are defined
- role-based login flow is implemented for the main user types
- admin, lecturer, and student privilege screens are available
- the project is now a working demonstration of the LMS role-based access model

## Demo login accounts

- Administrator: username = admin, password = admin123
- Lecturer: username = lecturer, password = lecturer123
- Student: username = student, password = student123

## Planned next steps

- add repository classes for users, courses, assignments, and submissions
- enforce role-based permissions in service methods
- implement late penalty, extension, and moderation logic
- add automated tests using GoogleTest or Catch2
- add static analysis and build verification
