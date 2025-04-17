// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#pragma once

#include <netinet/in.h>

class InetAdress {
public:
  InetAdress();
  InetAdress(const char *ip, uint16_t port);
  InetAdress(const sockaddr_in &addr);
  ~InetAdress() = default;

public:
  sockaddr *GetSockAddr() const;
  void SetSockAddr(const sockaddr_in &addr);
  const char *GetSockIp() const;
  uint16_t GetSockPort() const;

private:
  sockaddr_in addr_;
};
