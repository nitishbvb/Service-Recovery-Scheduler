#include <iostream>
#include <memory>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "scheduler.grpc.pb.h"

using namespace grpc;
using namespace recoveryscheduler;

// Service implements the recovery receiver interface
class RecoveryAgentImpl final : public ServiceRecoveryAgent::Service {
public:
    Status ExecuteRecovery(ServerContext* ctx, const RecoveryRequest* req, RecoveryResponse* res) override {
        std::cout << "[Service Agent] Received REMOTE Command to take Action ID: " << req->action() << "\n";
        // Perform local structural changes (clear buffers, reset sockets, exit)
        res->set_success(true);
        return Status::OK;
    }
};

void run_agent_server() {
    std::string agent_address("0.0.0.0:6001");
    RecoveryAgentImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(agent_address, InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    server->Wait();
}

int main() {
    // 1. Run local recovery receiver server context in a dedicated background worker
    std::thread serverThread(run_agent_server);

    // 2. Connect to central Scheduler Process
    auto channel = CreateChannel("localhost:50051", InsecureChannelCredentials());
    auto stub = SchedulerService::NewStub(channel);

    // 3. Register self at startup
    RegisterRequest regReq;
    regReq.set_service_name("auth-service");
    regReq.set_target_address("localhost:6001"); // Where scheduler can reach us back
    regReq.add_recovery_sequence(ActionType::RESTART);
    regReq.add_recovery_sequence(ActionType::DISABLE);

    RegisterResponse regRes;
    ClientContext regCtx;
    stub->RegisterService(&regCtx, regReq, &regRes);

    // 4. Simulate hitting an issue later down the road
    std::cout << "[Service Logic] Hit unrecoverable file state lock! Notifying master scheduler...\n";
    FailureRequest failReq;
    failReq.set_service_name("auth-service");
    FailureResponse failRes;
    ClientContext failCtx;
    
    stub->NotifyFailure(&failCtx, failReq, &failRes);

    serverThread.join();
    return 0;
}
