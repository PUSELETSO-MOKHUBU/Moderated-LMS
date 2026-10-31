# Moderated LMS v1.1 - Technician Installation Guide

This guide installs the tested C++ application on one school-owned 64-bit Windows 10/11 workstation. The v1.1 system is a local console application backed by SQLite; it is not a web server and does not support multiple workstation clients sharing a live database.

## 1. Requirements

- A supported, updated Windows 10/11 x64 computer.
- An approved copy of the `v1.1` source repository and permission to install software.
- Internet access during setup to obtain the compiler dependencies and Catch2 test dependency.
- MSYS2 with its UCRT64 environment, Git, CMake, GCC, SQLite 3 and OpenSSL 3.
- A writable local data folder with sufficient space for the database and uploaded documents.
- Applications associated with PDF, Office/OpenDocument and text formats if staff need to open uploaded files.

Do not install the data folder under `Program Files`, a synchronized consumer folder, a removable drive, or a network share. SQLite and the upload files must be backed up together.

## 2. Install the build tools

1. Download and install MSYS2 from its official website using the school's software-installation policy.
2. Open **MSYS2 UCRT64** from the Start menu. Update the package database as instructed by the installer. If it asks you to close the terminal, close it, reopen **MSYS2 UCRT64**, and repeat the update command.
3. Install the required packages:

   ```sh
   pacman -S --needed git mingw-w64-ucrt-x86_64-toolchain \
     mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sqlite3 \
     mingw-w64-ucrt-x86_64-openssl
   ```

4. Confirm the tools are available:

   ```sh
   g++ --version
   cmake --version
   ```

Keep the MSYS2 UCRT64 runtime directory available when launching the executable so the SQLite, OpenSSL and compiler runtime DLLs can be found.

## 3. Obtain, build and test the release

In **MSYS2 UCRT64**, obtain the source and check out the release tag:

```sh
git clone https://github.com/PUSELETSO-MOKHUBU/Moderated-LMS.git
cd Moderated-LMS
git checkout v1.1
```

If GitHub requests authentication, sign in with an account authorized for the repository; never put a password or access token in the clone command. Then configure and build from the checked-out project directory:

```sh
cmake -S . -B build -G "MinGW Makefiles" -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The configure step downloads Catch2 when it is not already cached, so first-time setup needs network access. Do not proceed to deployment if any build or test command fails. The application executable is `build/ModeratedLMS.exe`.

## 4. Set up application and data folders

1. Install the checked-out release under a read-only application folder, for example `C:\SchoolApps\ModeratedLMS-v1.1`. Keep the executable and its required runtime DLLs together, or ensure the MSYS2 UCRT64 `bin` folder is on the launch environment's `PATH`.
2. Create a separate writable data folder, for example `C:\ProgramData\ModeratedLMS\Data`.
3. Grant **Modify** permission on the data folder only to the Windows account or school-managed group authorized to use the LMS. Avoid granting access to `Everyone`.
4. Create a Windows shortcut to `ModeratedLMS.exe`. In its shortcut properties set **Start in** to the data folder (for example `C:\ProgramData\ModeratedLMS\Data`). This is essential: the application creates `lms.db` and its `uploads` folders relative to its working directory.
5. Launch the shortcut once. Verify the application reaches its login prompt. The database and upload directories should be created under the data folder.
6. Sign in with the initial administrator account listed in the [README](../README.md#first-run-demonstration-accounts). Immediately create named accounts for authorized users, change the demonstration administrator password through **Edit/activate/deactivate user**, and disable unused demonstration accounts. Do not distribute demonstration credentials.
7. Verify a test login for each role and confirm that a test upload appears under the expected upload directory. Remove the test records/documents before operational use if they are not wanted.

The application stores password hashes rather than plaintext passwords. Enforce school password and account-handling policy independently; the program does not integrate with Windows/Active Directory authentication.

## 5. Routine operation and backup

- Users launch the configured shortcut and log in using their LMS account. The application is interactive and should remain open while in use.
- Keep the executable and the data folder separate. Never move or rename `lms.db` without moving the associated uploads and updating the shortcut's **Start in** folder.
- For a consistent backup, ask all users to log out and close the application, then copy **both** `lms.db` and the complete `uploads` directory to an approved, access-controlled backup location. Retain the same folder structure.
- To restore, close the application, restore the database and uploads together to the data directory, verify its access permissions, then launch and inspect the records and documents.
- Periodically test a restore to a separate folder. Retain backups according to the school's data-retention policy.
- Do not run independent copies of the application against one live data folder from different workstations. This release has no server-mediated concurrency, remote access, automated backup, or built-in account recovery.

## 6. Troubleshooting and handover

| Symptom | Technician action |
|---|---|
| Executable reports a missing DLL | Launch with MSYS2 UCRT64 `bin` on `PATH`, or deploy the required runtime DLLs from the approved toolchain beside the executable. Do not download DLLs from unofficial sites. |
| Database or upload creation fails | Check the shortcut's **Start in** value and the authorized user's Modify permission on the data folder. |
| Configure cannot download Catch2 | Check approved internet/proxy access, then rerun the configure command. Catch2 is needed for the test build, not for normal application use. |
| Uploaded document will not open | Confirm its format is supported and a trusted Windows application is installed and associated with that file type. |
| Login is rejected | Confirm username, password and active status with the administrator. The application does not provide password recovery. |

Record the installed release tag, executable location, data location, authorized Windows group, backup location and test/restore date in the school's asset and operations records. Do not record user passwords in the handover.
