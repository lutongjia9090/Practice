// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "inet_adress.h"
#include <arpa/inet.h>

InetAdress::InetAdress() {}

InetAdress::InetAdress(const char *ip, uint16_t port) {
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = inet_addr(ip);
  addr_.sin_port = htons(port);
}

InetAdress::InetAdress(const sockaddr_in &addr) : addr_(addr) {}

sockaddr *InetAdress::GetSockAddr() const { return (sockaddr *)&addr_; }

const char *InetAdress::GetSockIp() const { return inet_ntoa(addr_.sin_addr); }

uint16_t InetAdress::GetSockPort() const { return ntohs(addr_.sin_port); }

void InetAdress::SetSockAddr(const sockaddr_in &addr) { addr_ = addr; }
