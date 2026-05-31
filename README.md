# 🪐 Service Recovery Scheduler

A lightweight, platform-independent C++20 library and runtime framework that manages cascading recovery actions for monitored services.

---

##  How it Works

The system uses a **decoupled, file-based Inter-Process Communication (IPC)** model. Processes communicate by dropping simple files inside a shared `ipc_mailbox` folder.

*   **Core Library (`recovery_scheduler`)**: A pure in-memory state engine protected by thread-safe locks (`std::shared_mutex`). It handles all recovery logic and is 100% unit-testable.
*   **Server Process (`scheduler_server`)**: A background daemon that watches the mailbox directory for failure signals and updates the core library state.
*   **Client Process (`mock_service_client`)**: A separate standalone app that simulates crashes by dropping temporary flag files into the mailbox.

---

##  Prerequisites

*   **Compiler**: Any compiler supporting the C++20 standard (like Visual Studio 2022 on Windows).
*   **Testing Framework**: Prebuilt GoogleTest installed at `C:\gRPC_Libs\googletest`.

---

##  How to Build and Run in VS Code

1.  **Open Project**: Open the root directory of this project in VS Code.
2.  **Select Variant**: On the bottom status bar, set your build variant to **Debug** (to match your prebuilt GoogleTest profile).
3.  **Build**: Press **`F7`** to build the entire project.
4.  **Run the Tests**: Click the **Testing** icon on the left sidebar to discover and run the `scheduler_tests` suite.

---

##  Running the Multi-Process System

Because the Scheduler and the Client run as separate applications, you need to open **two separate terminal windows** to watch them talk to each other.

### Terminal 1: Start the Scheduler Server
Navigate to your project directory and launch the main backend daemon:
```powershell
.\build\bin\Debug\scheduler_server.exe
```
*(Output: `[Scheduler Server] Monitoring file-queue path: ...`)*

### Terminal 2: Start the Mock Service Client
In a second terminal window, run the client simulation app:
```powershell
.\build\bin\Debug\mock_service_client.exe
```

###  The Execution Loop
1.  The **Client** experiences a simulated error and drops an empty file named `auth-service.fail` into the mailbox folder.
2.  The **Server** detects the file, increments the recovery escalation level in the core library, and writes out a persistent `auth-service.state` file containing the new state.
3.  The **Server** deletes the temporary `.fail` file to acknowledge it.
4.  The **Client** reads the updated `.state` file from disk and confirms the action (e.g., `RESTART` or `DISABLE`).

---

##  Key Design Highlights

*   **Zero Dependency Friction**: By using native C++20 `std::filesystem` as a message queue, we completely avoided complex network linking errors and missing runtime `.dll` issues on Windows.
*   **Thread Safety**: Internal state storage is fully guarded with `std::shared_mutex`. Multiple threads can query service states simultaneously without blocking, while write/failure events run in strict isolation.
*   **Separation of Concerns**: The core recovery engine has no knowledge of how messages travel. It is written as a pure, decoupled C++ library target that can be easily dropped into any future project.
