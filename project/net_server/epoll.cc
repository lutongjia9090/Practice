// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "epoll.h"
#include "channel.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

Epoll::Epoll() {
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    printf("Failed to create epoll.");
    exit(1);
  }
}

Epoll::~Epoll() { close(epoll_fd_); }

int Epoll::GetFd() const { return epoll_fd_; }

void Epoll::UpdateChannel(Channel *ch) {
  epoll_event ev;
  ev.events = ch->GetEvent();
  ev.data.ptr = ch;

  if (ch->IsInepool()) {
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, ch->GetFd(), &ev) == -1) {
      printf("Failed to modify channel.");
      exit(-1);
    }
  } else {
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, ch->GetFd(), &ev) == -1) {
      printf("Failed to add channel.");
      exit(-1);
    }
    ch->SetInEpoll();
  }
}
std::vector<Channel *> Epoll::Loop(int timeout) {
  std::vector<Channel *> channels;
  bzero(evs_, sizeof(evs_));
  int fds = epoll_wait(epoll_fd_, evs_, MAX_EVENTS, timeout);
  if (fds < 0) {
    printf("epoll_wait failed.\n");
    exit(-1);
  }

  if (fds == 0) {
    printf("epoll_wait timeout.\n");
    return channels;
  }

  for (int i = 0; i < fds; ++i) {
    Channel *ch = (Channel *)evs_[i].data.ptr;
    ch->SetREvent(evs_[i].events);
    channels.emplace_back(ch);
  }
  return channels;
}
