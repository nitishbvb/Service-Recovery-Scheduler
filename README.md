
# 🪐 Cross-Platform Service Recovery Scheduler (gRPC)

A modern, platform-independent C++20 library and runtime framework that manages cascading recovery actions for monitored network microservices using gRPC.

---

## 🏗️ Architectural Blueprint

The platform decouples business logic, state tracking, and network transport using a bi-directional **gRPC Event Gateway** pattern.

|          SERVICE PROCESS           |             |         SCHEDULER PROCESS          ||                                    |             |                                    || +--------------------------------+ |   gRPC RPC  | +--------------------------------+ || |       Service gRPC Server      | | <=========  | |    Scheduler gRPC Server       | || | (Implements Recovery Steps)    | |             | | (Receives Failure Signals)     | || +--------------------------------+ |             | +--------------------------------+ ||                 │                  |             |                 │                  ||                 ▼ Calls            |   gRPC RPC  |                 ▼ Dispatches       ||          [NotifyFailure]           | ==========> |     [Core Library Engine]          |+------------------------------------+             +------------------------------------+


*   **Core Engine Library (`recovery_scheduler`)**: A pure in-memory data tracking module protected by internal reader-writer locks (`std::shared_mutex`). It handles state evaluation without any network code dependencies, making it 100% unit-testable.
*   **Scheduler Daemon (`scheduler_server`)**: A runnable process host encapsulating the core library. It listens for inbound failures from target applications and makes outbound gRPC client calls to trigger remote recovery operations.
*   **Recovery Agent (`mock_service_client`)**: A separate process template that monitors application conditions, updates the scheduler server, and exposes an endpoint to process automated recovery actions (e.g., `RESTART`, `STOP`, `DISABLE`).

---

## 🛠️ Machine Prerequisites

Ensure your machine has a C++20 toolchain and gRPC/Protobuf dependencies installed:

### 🪟 On Windows
*   **Compiler**: Visual Studio 2022 (with "Desktop development with C++" workload installed).
*   **Package Manager**: Install `grpc` and `protobuf` via **vcpkg**:
    ```powershell
    vcpkg install grpc protobuf
    ```

### 🐧 On Linux (Ubuntu/Debian)
*   **Compiler**: GCC 11+ or Clang 13+
*   **Dependencies**:
    ```bash
    sudo apt install build-essential cmake libgrpc++-dev protobuf-compiler-grpc
    ```

---

## 💻 Working Inside VS Code (Any OS)

This project fully supports the native VS Code CMake workflow:

1.  **Install Extensions**: Install **C/C++** and **CMake Tools** from the extensions marketplace.
2.  **Open Project**: Open the root directory of this project in VS Code.
3.  **Select a Kit**: Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS), type `CMake: Select a Kit`, and choose your C++20 compiler.
4.  **Configure & Build**: Press `F7` (or click **Build** on the bottom status bar). CMake will automatically invoke `protoc` and generate the network stub codes behind the scenes.
5.  **Run Tests**: Click the **Testing** icon on the left sidebar to discover and execute the GoogleTest suite.

---

## ⚡ Running the Multi-Process System

Because the Scheduler and the Service run as completely independent processes, you must open **two separate terminal windows** to watch them interact.

### 1️⃣ Terminal 1: Run the Scheduler Server
Navigate to your build output directory and start the central orchestrator:

```bash
# Windows (PowerShell/CMD)
.\build\bin\Debug\scheduler_server.exe

# Linux / macOS
./build/bin/scheduler_server
```
*(Expected Output: `[Scheduler Server] Listening across platform boundary on 0.0.0.0:50051`)*

### 2️⃣ Terminal 2: Run the Monitored Service Application
In a separate terminal tab, spin up the mock service:

```bash
# Windows (PowerShell/CMD)
.\build\bin\Debug\mock_service_client.exe

# Linux / macOS
./build/bin/mock_service_client
```

### 🔄 What Happens Next?
1.  The **Service** starts up and exposes its own recovery server endpoint on port `6001`.
2.  The **Service** sends a `RegisterService` gRPC network message to the central scheduler process (port `50051`) uploading its fallback timeline configuration (`RESTART → DISABLE`).
3.  The **Service** encounters a simulated error and calls `NotifyFailure`.
4.  The **Scheduler Daemon** captures the signal, references its local library data state, and **sends a remote network command back** commanding the application thread to execute an active `RESTART`.

---

## 🧪 Testing the Core Library

To execute the unit test runner in isolation without spinning up network ports or sockets:

```bash
cd build
ctest --output-on-failure
```

---

## 💡 Key Design Decisions & Justifications

*   **Platform Independence**: Abandoned UNIX named pipes/sockets in favour of gRPC. This allows the scheduling application to compile natively on Windows, macOS, or Linux, and communicate across completely different cloud nodes over standard TCP.
*   **Extensibility**: Traditional stdout logging or exit codes do not allow active management. By enforcing a bi-directional gRPC channel, the scheduler actively remediates the target application dynamically.
*   **Separation of Concerns**: The core engine library (`recovery_scheduler`) has zero dependency on gRPC headers. This ensures your logical state changes can be checked in microsecond speeds through unit tests without standing up real servers.
*   **Thread Safety**: Live tracking blocks use `std::shared_mutex`. Multiple parallel reader threads can call `queryState()` concurrently, while failure write steps are held in strict exclusive isolation.
