# 🏛️ Technical Architecture Specification

This document outlines the engineering patterns, thread-safety guarantees, and architectural choices behind the Service Recovery Scheduler.

---

## 🎯 System Design Goals
1.  **Strict Separation of Concerns**: The scheduling library must remain pure C++, entirely decoupled from networking protocols, file paths, or process orchestration hooks.
2.  **Platform Independence**: Avoid platform-locked primitives (like Linux-only POSIX Named Pipes or Windows-only Named Pipes) to ensure seamless compilation across modern operating systems.
3.  **Zero Dependency Friction**: Mitigate compilation version mismatches, runtime `.dll` path injection loops, and toolchain synchronization errors typical of complex RPC runtimes (e.g., gRPC/Protobuf) on enterprise Windows environments.

---

## 🧬 Architectural Layer Separation

The project enforces strict decoupling by splitting system responsibilities across three distinct boundaries: The Core Logic Tier, The File-IPC Gateway, and The Independent App Runtimes. By separating calculation from communication, the platform remains highly visible, easily testable, and completely insulated from cross-process memory corruption.

### 1. The Core Library Layer (`recovery_scheduler`)
Acts as a thread-safe state ledger. It maintains an immutable registration configuration map and an active live state map tracking the progress of services through their recovery tracks.

### 2. The Shared Mailbox IPC Layer
The local file system is leveraged as an asynchronous, transaction-based message queue utilizing native C++20 `std::filesystem`.
*   **Inbound Failure Channels (`*.fail` files)**: Functions exactly like a queue push. The presence of a file notifies the host that an event occurred. Once parsed, the host deletes the file, behaving like a queue item consumption (`pop`).
*   **Outbound Action Channels (`*.state` files)**: Serves as the query and telemetry contract. It broadcasts what action was commanded and the service's current escalation tier index.

### 3. Process Runtime Targets (apps/ binaries)
The final layer consists of the standalone execution binaries running in isolated memory spaces. The scheduler_server daemon manages the host thread pool, driving the file monitoring loop on its main thread while hosting an interactive user terminal query thread via a reader lock (std::shared_lock). On the other side, the mock_service_client manages the target application. It handles user inputs to trigger simulated failures on a background thread, while its main thread monitors file write times to read incoming scheduler actions and execute localized dummy prints.

---

## 🔒 Thread Safety & Memory Synchronization

To handle concurrent real-world infrastructure operations where multiple tasks might crash simultaneously while diagnostics poll live states, the core engine implements **Reader-Writer Isolation** via **`std::shared_mutex`**.

```cpp
mutable std::shared_mutex m_mutex;
```

1.  **Exclusive Write Lock (`std::unique_lock`)**: Utilized inside `registerService()` and `processFailure()`. When an inbound failure file is processed, the writing thread forces exclusive isolation. No other thread can read or write to the tracking map, preventing race conditions or data corruption.
2.  **Shared Read Lock (`std::shared_lock`)**: Utilized inside `queryServiceState()`. This allows the terminal interactive user thread to read tracking data simultaneously with other component readers without stalling or introducing thread block locks.

---

## ⚙️ Key Patterns & Operational Safeguards

### 1. Atomic Snapshot File Swaps
To ensure that an external reader process (like the client application) never parses a half-written or corrupted state file while the scheduler daemon is flushing data to disk, updates use an atomic swap pattern:
1.  The scheduler writes the tracking configuration data out to a temporary hidden file: `auth-service.state.tmp`.
2.  The file handle is safely flushed and closed.
3.  The scheduler invokes **`std::filesystem::rename`**. At the operating system kernel level, this swap is executed as an atomic pointer reallocation. The client process sees either the complete old file or the complete new file, guaranteeing transaction safety.

### 2. Saturation and Boundary Capping
When a service continues to crash after reaching the end of its registered recovery array, the engine prevents index-out-of-bounds corruption. It caps the index at the final index slot ($N-1$), maintaining the final defensive recovery state (e.g., keeping the service locked in a `DISABLE` state) rather than overflowing or throwing exceptions.

### 3. Separation of Transport Logic
By separating file parsing code into the execution binaries (`main.cpp` and `service_main.cpp`), the underlying core engine library maintains zero dependencies on directory trees or string parsers. This structural isolation allows the core state machine library to be integrated into any future network socket, RPC, or message broker framework with zero modifications.

