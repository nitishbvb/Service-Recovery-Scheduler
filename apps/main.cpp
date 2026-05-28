#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "scheduler.grpc.pb.h"
#include <recovery_scheduler/scheduler.hpp>

using namespace grpc;
using namespace recoveryscheduler;

class SchedulerServiceImpl final : public SchedulerService::Service {
private:
    RecoveryScheduler m_coreEngine;

    // Utility map to cleanly bridge Protobuf enums to standard C++ enums
    RecoveryAction mapEnum(ActionType act) {
        return static_cast<RecoveryAction>(act);
    }

public:
    Status RegisterService(ServerContext* ctx, const RegisterRequest* req, RegisterResponse* res) override {
        std::vector<RecoveryAction> seq;
        for (int i = 0; i < req->recovery_sequence_size(); ++i) {
            seq.push_back(mapEnum(req->recovery_sequence(i)));
        }
        m_coreEngine.registerService(req->service_name(), req->target_address(), seq);
        res->set_success(true);
        return Status::OK;
    }

    Status NotifyFailure(ServerContext* ctx, const FailureRequest* req, FailureResponse* res) override {
        auto outcome = m_coreEngine.processFailure(req->service_name());
        if (!outcome) {
            res->set_success(false);
            return Status::NOT_FOUND;
        }

        auto [targetAddress, actionToTake] = *outcome;

        // EXTENSIBILITY LINK: Actively command the service process to execute recovery
        auto channel = CreateChannel(targetAddress, InsecureChannelCredentials());
        auto stub = ServiceRecoveryAgent::NewStub(channel);

        RecoveryRequest recoveryReq;
        recoveryReq.set_action(static_cast<ActionType>(actionToTake));
        RecoveryResponse recoveryRes;
        ClientContext clientCtx;

        Status rpcStatus = stub->ExecuteRecovery(&clientCtx, recoveryReq, &recoveryRes);

        res->set_action_dispatched(static_cast<ActionType>(actionToTake));
        res->set_success(rpcStatus.ok() && recoveryRes.success());
        return Status::OK;
    }
};

int main() {
    std::string server_address("0.0.0.0:50051");
    SchedulerServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "[Scheduler Server] Listening across platform boundary on " << server_address << "\n";
    server->Wait();
    return 0;
}
