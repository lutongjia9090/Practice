// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include "src/common/storage_engine.h"
#include "src/proto/kv_service.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace tiny_kv {

class AsyncGrpcKVServer;

/************************************************************************/
/* BaseServiceContext */
/************************************************************************/
template <typename Request, typename Response>
class BaseServiceContext {
protected:
  Request request_;
  Response response_;
  grpc::ServerContext ctx_;
  KVService::AsyncService* service_;
  std::unique_ptr<StorageEngine>& storage_;
  grpc::ServerAsyncResponseWriter<Response> responder_;
  bool is_finished_ = false;

public:
  BaseServiceContext(std::unique_ptr<StorageEngine>& storage)
      : service_(nullptr), storage_(storage), responder_(&ctx_) {}

  virtual ~BaseServiceContext() = default;

  void set_service(KVService::AsyncService* service) {
    service_ = service;
  }

  virtual void DoRequest(grpc::ServerCompletionQueue* cq) = 0;
  virtual void Process() = 0;
  virtual void Recycle() = 0;
};

/************************************************************************/
/* GetServiceContext */
/************************************************************************/
class GetServiceContext : public BaseServiceContext<GetRequest, GetResponse> {
public:
  GetServiceContext(std::unique_ptr<StorageEngine>& storage);
  ~GetServiceContext() override = default;

  void DoRequest(grpc::ServerCompletionQueue* cq) override;
  void Process() override;
  void Recycle() override;

private:
  enum class Status { CREATE, PROCESS, FINISH };
  Status status_ = Status::CREATE;
  grpc::ServerCompletionQueue* cq_ = nullptr;
};

/************************************************************************/
/* PutServiceContext */
/************************************************************************/
class PutServiceContext : public BaseServiceContext<PutRequest, PutResponse> {
public:
  PutServiceContext(std::unique_ptr<StorageEngine>& storage);
  ~PutServiceContext() override = default;

  void DoRequest(grpc::ServerCompletionQueue* cq) override;
  void Process() override;
  void Recycle() override;

private:
  enum class Status { CREATE, PROCESS, FINISH };
  Status status_ = Status::CREATE;
  grpc::ServerCompletionQueue* cq_ = nullptr;
};

/************************************************************************/
/* DeleteServiceContext */
/************************************************************************/
class DeleteServiceContext : public BaseServiceContext<DeleteRequest, DeleteResponse> {
public:
  DeleteServiceContext(std::unique_ptr<StorageEngine>& storage);
  ~DeleteServiceContext() override = default;

  void DoRequest(grpc::ServerCompletionQueue* cq) override;
  void Process() override;
  void Recycle() override;

private:
  enum class Status { CREATE, PROCESS, FINISH };
  Status status_ = Status::CREATE;
  grpc::ServerCompletionQueue* cq_ = nullptr;
};

/************************************************************************/
/* AsyncKVServiceImpl */
/************************************************************************/
class AsyncKVServiceImpl {
public:
  explicit AsyncKVServiceImpl(const std::string& storage_type = "memory",
                             const std::string& storage_path = "");
  ~AsyncKVServiceImpl();

  void Start(const std::string& server_address, int num_threads);
  void Stop();
  void Wait();

private:
  void HandleRequests();
  void CreateContexts();

  std::unique_ptr<KVService::AsyncService> service_;
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<grpc::Server> server_;
  std::unique_ptr<StorageEngine> storage_;
  std::vector<std::thread> threads_;
  bool shutdown_ = false;
};

/************************************************************************/
/* AsyncGrpcKVServer */
/************************************************************************/
class AsyncGrpcKVServer {
public:
  AsyncGrpcKVServer(const std::string& server_address,
                   const std::string& storage_type = "memory",
                   const std::string& storage_path = "",
                   int num_threads = 4);
  ~AsyncGrpcKVServer();

  void Start();
  void Wait();
  void Stop();

private:
  std::string server_address_;
  std::unique_ptr<AsyncKVServiceImpl> service_;
  int num_threads_;
};

} // namespace tiny_kv
