// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "grpc_kv_client.h"

namespace tiny_kv {

GrpcKVClient::GrpcKVClient(const std::string &server_address)
    : server_address_(server_address), connected_(false) {}

GrpcKVClient::~GrpcKVClient() { Shutdown(); }

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
    last_error_ = "Not connected to server";
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
    last_error_ = "Not connected to server";
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
    last_error_ = "Not connected to server";
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

std::string GrpcKVClient::GetLastError() const { return last_error_; }

} // namespace tiny_kv
