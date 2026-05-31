# 🪐 Service Recovery Scheduler — Setup & Execution Guide

A platform-independent C++20 library and runtime system designed to manage cascading recovery actions for independent processes using a decoupled, file-based Inter-Process Communication (IPC) strategy.

---

## 🛠️ Prerequisites

Before compiling, make sure your machine has the following components ready:
*   **Compiler**: Any compiler supporting the C++20 standard (e.g., Visual Studio 2022 on Windows).
*   **Build Automation**: CMake version 3.20 or newer.
*   **Testing Library**: Prebuilt GoogleTest library compiled in **Debug** mode, placed at `C:\gRPC_Libs\googletest`.

---

## 💻 Building the Project in VS Code

This repository fully integrates with the official VS Code CMake workflow:

1.  **Open the Workspace**: Open the root directory of the project in VS Code.
2.  **Select your Compiler**: Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS), type `CMake: Select a Kit`, and select your C++20 toolchain (e.g., *Visual Studio Community 2022 Release - amd64*).
3.  **Set the Build Variant**: Look at the VS Code bottom status bar and ensure the build type is set explicitly to **Debug** (this ensures alignment with the prebuilt GoogleTest runtime library flags).
4.  **Compile the Binaries**: Press **`F7`** (or click **Build** on the status bar). CMake will compile the core library, the test suite, and both execution applications.

---

## ⚡ Running the Multi-Process System

Because the Scheduler and the Monitored Service Client run as separate, independent processes, you need to open **two separate terminal panels** side-by-side inside VS Code.

### 1️⃣ Terminal 1: Launch the Scheduler Server
Navigate to your project directory and start the central orchestrator daemon:
```powershell
.\build\bin\Debug\scheduler_server.exe
```
*(Expected Output: `[Scheduler Server] Monitoring file-queue path: .../ipc_mailbox`)*

**Interactive Query Mode**: This terminal features an active background thread. You can type the name of a registered service (e.g., `auth-service`) at any point and press Enter to instantly check its current level and last action taken from live memory.

### 2️⃣ Terminal 2: Launch the Monitored Service Client
In your second terminal window, spin up the standalone application client:
```powershell
.\build\bin\Debug\mock_service_client.exe
```
*(Expected Output: `[Service Client Process] Background listener loop is active. Awaiting actions...`)*

### 🔄 Testing the Interactive Failure Loop
1. Go to **Terminal 2 (Client)**, type **`fail`**, and press Enter.
2. The client will drop a temporary transaction token (`auth-service.fail`) into the mailbox folder.
3. Look at **Terminal 1 (Server)**: It will log that a failure was caught, increment the internal escalation level, and write out an atomic state file (`auth-service.state`).
4. Look back at **Terminal 2 (Client)**: It will instantly pick up the state modification, read the action type, and print a custom dummy recovery block on screen (e.g., executing standard `RESTART` hooks).
5. Type **`fail`** a second and third time inside Terminal 2 to watch the system dynamically escalate through the configuration path (`RESTART` ➡️ `RESTART` ➡️ `DISABLE`).

---

## 🧪 Running the Unit Test Suite

To verify the core engine's isolated logical tracking and thread-safety limits without any directory polling overhead, use the standard CTest wrapper:

```powershell
cd build
ctest --output-on-failure
```
*(Alternatively, click the **Testing Flask Icon** on the left sidebar of VS Code to debug tests line-by-line using breakpoints).*
