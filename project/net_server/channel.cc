// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "channel.h"

Channel::Channel(int fd, Epoll *ep) : fd_(fd), epoll_(ep) {}

// No distruct epoll_ or fd_, channel is only use them.
Channel::~Channel() {}

int Channel::GetFd() const { return fd_; }

void Channel::UseET() { events_ |= EPOLLET; }

void Channel::EnableReading() {
  events_ |= EPOLLIN;
  epoll_->UpdateChannel(this);
}

void Channel::SetInEpoll() { in_epoll_ = true; }

void Channel::SetREvent(uint32_t ev) { revents_ = ev; }

bool Channel::IsInepool() const { return in_epoll_; }

uint32_t Channel::GetEvent() const { return events_; }

uint32_t Channel::GetREvent() const { return revents_; }
