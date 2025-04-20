// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "async_grpc_kv_server.h"
#include <iostream>

namespace tiny_kv {

/************************************************************************/
/* GetServiceContext */
/************************************************************************/
GetServiceContext::GetServiceContext(std::unique_ptr<StorageEngine> &storage)
    : BaseServiceContext<GetRequest, GetResponse>(storage) {}

void GetServiceContext::DoRequest(grpc::ServerCompletionQueue *cq) {
  cq_ = cq;
  service_->RequestGet(&ctx_, &request_, &responder_, cq, cq, this);
}

void GetServiceContext::Process() {
  if (status_ == Status::CREATE) {
    auto *new_context = new GetServiceContext(storage_);
    new_context->set_service(service_);
    new_context->DoRequest(cq_);

    status_ = Status::PROCESS;

    auto value = storage_->Get(request_.key());
    if (value.has_value()) {
      response_.set_success(true);
      response_.set_message("success");
      response_.set_value(*value);
    } else {
      response_.set_success(false);
      response_.set_message("key not found");
      response_.set_value("");
    }

    responder_.Finish(response_, grpc::Status::OK, this);

  } else if (status_ == Status::PROCESS) {
    status_ = Status::FINISH;
    Recycle();
  }
}

void GetServiceContext::Recycle() {
  if (status_ == Status::FINISH) {
    delete this;
  }
}

/************************************************************************/
/* PutServiceContext */
/************************************************************************/
PutServiceContext::PutServiceContext(std::unique_ptr<StorageEngine> &storage)
    : BaseServiceContext<PutRequest, PutResponse>(storage) {}

void PutServiceContext::DoRequest(grpc::ServerCompletionQueue *cq) {
  cq_ = cq;
  service_->RequestPut(&ctx_, &request_, &responder_, cq, cq, this);
}

void PutServiceContext::Process() {
  if (status_ == Status::CREATE) {
    auto *new_context = new PutServiceContext(storage_);
    new_context->set_service(service_);
    new_context->DoRequest(cq_);

    status_ = Status::PROCESS;

    bool success = storage_->Put(request_.key(), request_.value());
    response_.set_success(success);
    response_.set_message(success ? "success" : "fail");

    responder_.Finish(response_, grpc::Status::OK, this);

  } else if (status_ == Status::PROCESS) {
    status_ = Status::FINISH;
    Recycle();
  }
}

void PutServiceContext::Recycle() {
  if (status_ == Status::FINISH) {
    delete this;
  }
}

/************************************************************************/
/* DeleteServiceContext */
/************************************************************************/
DeleteServiceContext::DeleteServiceContext(
    std::unique_ptr<StorageEngine> &storage)
    : BaseServiceContext<DeleteRequest, DeleteResponse>(storage) {}

void DeleteServiceContext::DoRequest(grpc::ServerCompletionQueue *cq) {
  cq_ = cq;
  service_->RequestDelete(&ctx_, &request_, &responder_, cq, cq, this);
}

void DeleteServiceContext::Process() {
  if (status_ == Status::CREATE) {
    auto *new_context = new DeleteServiceContext(storage_);
    new_context->set_service(service_);
    new_context->DoRequest(cq_);

    status_ = Status::PROCESS;

    bool success = storage_->Delete(request_.key());
    response_.set_success(success);
    response_.set_message(success ? "success" : "fail");

    responder_.Finish(response_, grpc::Status::OK, this);

  } else if (status_ == Status::PROCESS) {
    status_ = Status::FINISH;
    Recycle();
  }
}

void DeleteServiceContext::Recycle() {
  if (status_ == Status::FINISH) {
    delete this;
  }
}

/************************************************************************/
/* MultiGetServiceContext */
/************************************************************************/
MultiGetServiceContext::MultiGetServiceContext(std::unique_ptr<StorageEngine> &storage)
    : BaseServiceContext<MultiGetRequest, MultiGetResponse>(storage) {}

void MultiGetServiceContext::DoRequest(grpc::ServerCompletionQueue *cq) {
  cq_ = cq;
  service_->RequestMultiGet(&ctx_, &request_, &responder_, cq, cq, this);
}

void MultiGetServiceContext::Process() {
  if (status_ == Status::CREATE) {
    auto *new_context = new MultiGetServiceContext(storage_);
    new_context->set_service(service_);
    new_context->DoRequest(cq_);

    status_ = Status::PROCESS;

    response_.set_success(true);
    response_.set_message("success");

    for (const auto& key : request_.keys()) {
      auto value = storage_->Get(key);
      auto* kv = response_.add_kvs();
      kv->set_key(key);
      kv->set_value(value.has_value() ? *value : "");
    }

    responder_.Finish(response_, grpc::Status::OK, this);

  } else if (status_ == Status::PROCESS) {
    status_ = Status::FINISH;
    Recycle();
  }
}

void MultiGetServiceContext::Recycle() {
  if (status_ == Status::FINISH) {
    delete this;
  }
}

/************************************************************************/
/* MultiPutServiceContext */
/************************************************************************/
MultiPutServiceContext::MultiPutServiceContext(std::unique_ptr<StorageEngine> &storage)
    : BaseServiceContext<MultiPutRequest, MultiPutResponse>(storage) {}

void MultiPutServiceContext::DoRequest(grpc::ServerCompletionQueue *cq) {
  cq_ = cq;
  service_->RequestMultiPut(&ctx_, &request_, &responder_, cq, cq, this);
}

void MultiPutServiceContext::Process() {
  if (status_ == Status::CREATE) {
    auto *new_context = new MultiPutServiceContext(storage_);
    new_context->set_service(service_);
    new_context->DoRequest(cq_);

    status_ = Status::PROCESS;

    bool success = true;
    for (const auto& kv : request_.kvs()) {
      if (!storage_->Put(kv.key(), kv.value())) {
        success = false;
      }
    }

    response_.set_success(success);
    response_.set_message(success ? "success" : "partial failure");

    responder_.Finish(response_, grpc::Status::OK, this);

  } else if (status_ == Status::PROCESS) {
    status_ = Status::FINISH;
    Recycle();
  }
}

void MultiPutServiceContext::Recycle() {
  if (status_ == Status::FINISH) {
    delete this;
  }
}

/************************************************************************/
/* MultiDeleteServiceContext */
/************************************************************************/
MultiDeleteServiceContext::MultiDeleteServiceContext(std::unique_ptr<StorageEngine> &storage)
    : BaseServiceContext<MultiDeleteRequest, MultiDeleteResponse>(storage) {}

void MultiDeleteServiceContext::DoRequest(grpc::ServerCompletionQueue *cq) {
  cq_ = cq;
  service_->RequestMultiDelete(&ctx_, &request_, &responder_, cq, cq, this);
}

void MultiDeleteServiceContext::Process() {
  if (status_ == Status::CREATE) {
    auto *new_context = new MultiDeleteServiceContext(storage_);
    new_context->set_service(service_);
    new_context->DoRequest(cq_);

    status_ = Status::PROCESS;

    bool success = true;
    for (const auto& key : request_.keys()) {
      if (!storage_->Delete(key)) {
        success = false;
      }
    }

    response_.set_success(success);
    response_.set_message(success ? "success" : "partial failure");

    responder_.Finish(response_, grpc::Status::OK, this);

  } else if (status_ == Status::PROCESS) {
    status_ = Status::FINISH;
    Recycle();
  }
}

void MultiDeleteServiceContext::Recycle() {
  if (status_ == Status::FINISH) {
    delete this;
  }
}

/************************************************************************/
/* AsyncKVServiceImpl */
/************************************************************************/
AsyncKVServiceImpl::AsyncKVServiceImpl(const std::string &storage_type,
                                       const std::string &storage_path)
    : service_(std::make_unique<KVService::AsyncService>()),
      storage_(CreateStorageEngine(storage_type, storage_path)),
      shutdown_(false) {}

AsyncKVServiceImpl::~AsyncKVServiceImpl() {
  Stop();

  auto *file_storage = dynamic_cast<FileStorage *>(storage_.get());
  if (file_storage) {
    file_storage->Persist();
  }
}

void AsyncKVServiceImpl::Start(const std::string &server_address,
                               int num_threads) {
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(service_.get());

  cq_ = builder.AddCompletionQueue();

  server_ = builder.BuildAndStart();
  std::cout << "KV Storage Async Server started, listening on: "
            << server_address << std::endl;

  CreateContexts();

  for (int i = 0; i < num_threads; ++i) {
    threads_.emplace_back(&AsyncKVServiceImpl::HandleRequests, this);
  }
}

void AsyncKVServiceImpl::CreateContexts() {
  auto *get_context = new GetServiceContext(storage_);
  get_context->set_service(service_.get());
  get_context->DoRequest(cq_.get());

  auto *put_context = new PutServiceContext(storage_);
  put_context->set_service(service_.get());
  put_context->DoRequest(cq_.get());

  auto *delete_context = new DeleteServiceContext(storage_);
  delete_context->set_service(service_.get());
  delete_context->DoRequest(cq_.get());

  auto *multi_get_context = new MultiGetServiceContext(storage_);
  multi_get_context->set_service(service_.get());
  multi_get_context->DoRequest(cq_.get());

  auto *multi_put_context = new MultiPutServiceContext(storage_);
  multi_put_context->set_service(service_.get());
  multi_put_context->DoRequest(cq_.get());

  auto *multi_delete_context = new MultiDeleteServiceContext(storage_);
  multi_delete_context->set_service(service_.get());
  multi_delete_context->DoRequest(cq_.get());
}

void AsyncKVServiceImpl::HandleRequests() {
  void *tag;
  bool ok;

  while (!shutdown_) {
    bool got_event = cq_->Next(&tag, &ok);

    if (!got_event) {
      break;
    }

    if (ok) {
      auto *context =
          static_cast<BaseServiceContext<GetRequest, GetResponse> *>(tag);
      context->Process();
    }
  }
}

void AsyncKVServiceImpl::Stop() {
  if (!shutdown_) {
    shutdown_ = true;

    server_->Shutdown();
    cq_->Shutdown();

    Wait();
  }
}

void AsyncKVServiceImpl::Wait() {
  for (auto &thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  threads_.clear();
}

/************************************************************************/
/* AsyncGrpcKVServer */
/************************************************************************/
AsyncGrpcKVServer::AsyncGrpcKVServer(const std::string &server_address,
                                     const std::string &storage_type,
                                     const std::string &storage_path,
                                     int num_threads)
    : server_address_(server_address),
      service_(
          std::make_unique<AsyncKVServiceImpl>(storage_type, storage_path)),
      num_threads_(num_threads) {}

AsyncGrpcKVServer::~AsyncGrpcKVServer() { Stop(); }

void AsyncGrpcKVServer::Start() {
  service_->Start(server_address_, num_threads_);
}

void AsyncGrpcKVServer::Wait() { service_->Wait(); }

void AsyncGrpcKVServer::Stop() { service_->Stop(); }

} // namespace tiny_kv
