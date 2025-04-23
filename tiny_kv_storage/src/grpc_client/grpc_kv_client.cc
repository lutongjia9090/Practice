// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "grpc_kv_client.h"

namespace tiny_kv {

GrpcKVClient::GrpcKVClient(const std::string &server_address)
    : server_address_(server_address), connected_(false) {
  running_ = true;
  completion_thread_ = std::thread([this]() {
    void *tag;
    bool ok;
    while (cq_.Next(&tag, &ok)) {
      if (!running_)
        break;
      auto *callback = static_cast<std::function<void()> *>(tag);
      (*callback)();
      delete callback;
    }
  });
}

GrpcKVClient::~GrpcKVClient() {
  running_ = false;
  cq_.Shutdown();
  if (completion_thread_.joinable()) {
    completion_thread_.join();
  }
  Shutdown();
}

bool GrpcKVClient::Connect() {
  if (connected_) {
    return true;
  }

  channel_ =
      grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());

  stub_ = KVService::NewStub(channel_);

  if (stub_) {
    connected_ = true;
    last_error_.clear();
    if (!running_) {
      running_ = true;
    }
    return true;
  }

  last_error_ = "Failed to create gRPC stub";
  return false;
}

void GrpcKVClient::Shutdown() {
  if (connected_) {
    stub_.reset();
    channel_.reset();
    connected_ = false;
  }
}

std::pair<bool, std::string> GrpcKVClient::Get(const std::string &key) {
  if (!connected_) {
    last_error_ = "Failed to connect to server";
    return {false, ""};
  }

  GetRequest request;
  request.set_key(key);

  GetResponse response;

  grpc::ClientContext context;

  grpc::Status status = stub_->Get(&context, request, &response);

  if (!status.ok()) {
    last_error_ = "RPC failed: " + status.error_message();
    return {false, ""};
  }

  if (!response.success()) {
    last_error_ = response.message();
    return {false, ""};
  }

  return {true, response.value()};
}

bool GrpcKVClient::Put(const std::string &key, const std::string &value) {
  if (!connected_) {
    last_error_ = "Failed to connect to server";
    return false;
  }

  PutRequest request;
  request.set_key(key);
  request.set_value(value);

  PutResponse response;

  grpc::ClientContext context;

  grpc::Status status = stub_->Put(&context, request, &response);

  if (!status.ok()) {
    last_error_ = "RPC failed: " + status.error_message();
    return false;
  }

  if (!response.success()) {
    last_error_ = response.message();
    return false;
  }

  return true;
}

bool GrpcKVClient::Delete(const std::string &key) {
  if (!connected_) {
    last_error_ = "Failed to connect to server";
    return false;
  }

  DeleteRequest request;
  request.set_key(key);

  DeleteResponse response;

  grpc::ClientContext context;

  grpc::Status status = stub_->Delete(&context, request, &response);

  if (!status.ok()) {
    last_error_ = "RPC failed: " + status.error_message();
    return false;
  }

  if (!response.success()) {
    last_error_ = response.message();
    return false;
  }

  return true;
}

std::unordered_map<std::string, std::string>
GrpcKVClient::MultiGet(const std::vector<std::string> &keys) {
  std::unordered_map<std::string, std::string> result;

  if (!connected_) {
    last_error_ = "Failed to connect to server";
    return result;
  }

  MultiGetRequest request;
  for (const auto &key : keys) {
    request.add_keys(key);
  }

  MultiGetResponse response;
  grpc::ClientContext context;

  grpc::Status status = stub_->MultiGet(&context, request, &response);

  if (!status.ok()) {
    last_error_ = "RPC failed: " + status.error_message();
    return result;
  }

  if (!response.success()) {
    last_error_ = response.message();
    return result;
  }

  for (const auto &kv : response.kvs()) {
    result[kv.key()] = kv.value();
  }

  return result;
}

bool GrpcKVClient::MultiPut(
    const std::unordered_map<std::string, std::string> &kv_pairs) {
  if (!connected_) {
    last_error_ = "Failed to connect to server";
    return false;
  }

  MultiPutRequest request;
  for (const auto & [ key, value ] : kv_pairs) {
    auto *kv = request.add_kvs();
    kv->set_key(key);
    kv->set_value(value);
  }

  MultiPutResponse response;
  grpc::ClientContext context;

  grpc::Status status = stub_->MultiPut(&context, request, &response);

  if (!status.ok()) {
    last_error_ = "RPC failed: " + status.error_message();
    return false;
  }

  if (!response.success()) {
    last_error_ = response.message();
    return false;
  }

  return true;
}

bool GrpcKVClient::MultiDelete(const std::vector<std::string> &keys) {
  if (!connected_) {
    last_error_ = "Failed to connect to server";
    return false;
  }

  MultiDeleteRequest request;
  for (const auto &key : keys) {
    request.add_keys(key);
  }

  MultiDeleteResponse response;
  grpc::ClientContext context;

  grpc::Status status = stub_->MultiDelete(&context, request, &response);

  if (!status.ok()) {
    last_error_ = "RPC failed: " + status.error_message();
    return false;
  }

  if (!response.success()) {
    last_error_ = response.message();
    return false;
  }

  return true;
}

std::string GrpcKVClient::GetLastError() const { return last_error_; }

void GrpcKVClient::AsyncGet(
    const std::string &key,
    std::function<void(bool, const std::string &)> callback) {
  if (!connected_) {
    callback(false, "Failed to connect to server");
    return;
  }

  auto *ctx = new grpc::ClientContext;
  auto *request = new GetRequest;
  auto *response = new GetResponse;
  request->set_key(key);

  auto *call = new std::function<void()>([=]() {
    bool success = false;
    std::string value;
    if (response->success()) {
      success = true;
      value = response->value();
    }
    callback(success, value);
    delete ctx;
    delete request;
    delete response;
  });

  stub_->async()->Get(ctx, request, response, [call](grpc::Status /*status*/) {
    (*call)();
    delete call;
  });
}

void GrpcKVClient::AsyncPut(const std::string &key, const std::string &value,
                            std::function<void(bool)> callback) {
  if (!connected_) {
    callback(false);
    return;
  }

  auto *ctx = new grpc::ClientContext;
  auto *request = new PutRequest;
  auto *response = new PutResponse;
  request->set_key(key);
  request->set_value(value);

  auto *call = new std::function<void()>([=]() {
    bool success = response->success();
    callback(success);
    delete ctx;
    delete request;
    delete response;
  });

  stub_->async()->Put(ctx, request, response, [call](grpc::Status /*status*/) {
    (*call)();
    delete call;
  });
}

void GrpcKVClient::AsyncDelete(const std::string &key,
                               std::function<void(bool)> callback) {
  if (!connected_) {
    callback(false);
    return;
  }

  auto *ctx = new grpc::ClientContext;
  auto *request = new DeleteRequest;
  auto *response = new DeleteResponse;
  request->set_key(key);

  auto *call = new std::function<void()>([=]() {
    bool success = response->success();
    callback(success);
    delete ctx;
    delete request;
    delete response;
  });

  stub_->async()->Delete(ctx, request, response,
                         [call](grpc::Status /*status*/) {
                           (*call)();
                           delete call;
                         });
}

void GrpcKVClient::AsyncMultiGet(
    const std::vector<std::string> &keys,
    std::function<void(bool,
                       const std::unordered_map<std::string, std::string> &)>
        callback) {
  if (!connected_) {
    callback(false, {});
    return;
  }

  auto *ctx = new grpc::ClientContext;
  auto *request = new MultiGetRequest;
  auto *response = new MultiGetResponse;

  for (const auto &key : keys) {
    request->add_keys(key);
  }

  auto *call = new std::function<void()>([=]() {
    std::unordered_map<std::string, std::string> result;
    bool success = response->success();

    if (success) {
      for (const auto &kv : response->kvs()) {
        result[kv.key()] = kv.value();
      }
    }

    callback(success, result);
    delete ctx;
    delete request;
    delete response;
  });

  stub_->async()->MultiGet(ctx, request, response,
                           [call](grpc::Status /*status*/) {
                             (*call)();
                             delete call;
                           });
}

void GrpcKVClient::AsyncMultiPut(
    const std::unordered_map<std::string, std::string> &kv_pairs,
    std::function<void(bool)> callback) {
  if (!connected_) {
    callback(false);
    return;
  }

  auto *ctx = new grpc::ClientContext;
  auto *request = new MultiPutRequest;
  auto *response = new MultiPutResponse;

  for (const auto & [ key, value ] : kv_pairs) {
    auto *kv = request->add_kvs();
    kv->set_key(key);
    kv->set_value(value);
  }

  auto *call = new std::function<void()>([=]() {
    bool success = response->success();
    callback(success);
    delete ctx;
    delete request;
    delete response;
  });

  stub_->async()->MultiPut(ctx, request, response,
                           [call](grpc::Status /*status*/) {
                             (*call)();
                             delete call;
                           });
}

void GrpcKVClient::AsyncMultiDelete(const std::vector<std::string> &keys,
                                    std::function<void(bool)> callback) {
  if (!connected_) {
    callback(false);
    return;
  }

  auto *ctx = new grpc::ClientContext;
  auto *request = new MultiDeleteRequest;
  auto *response = new MultiDeleteResponse;

  for (const auto &key : keys) {
    request->add_keys(key);
  }

  auto *call = new std::function<void()>([=]() {
    bool success = response->success();
    callback(success);
    delete ctx;
    delete request;
    delete response;
  });

  stub_->async()->MultiDelete(ctx, request, response,
                              [call](grpc::Status /*status*/) {
                                (*call)();
                                delete call;
                              });
}

} // namespace tiny_kv
