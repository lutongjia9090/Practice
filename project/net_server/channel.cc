// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "channel.h"
#include "inet_adress.h"
#include <cerrno>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

Channel::Channel(int fd, Epoll *ep, bool is_listen)
    : fd_(fd), epoll_(ep), is_listen_(is_listen) {}

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

void Channel::HandleEvent(Socket *server_sock) {
  // client close, use `EPOLLIN` in some systems(recv() return 0)
  if (revents_ & EPOLLRDHUP) {
    printf("Client(event fd=%d) disconnected.", fd_);
    close(fd_);
  } else if (revents_ & EPOLLIN) {
    if (is_listen_) { // listen event
      InetAdress client_addr;
      Socket *client_sock = new Socket(server_sock->Accept(client_addr));

      Channel *client_ch = new Channel(client_sock->GetFd(), epoll_, false);
      client_ch->UseET();
      client_ch->EnableReading();
    } else { // read event
      char buffer[1024];
      while (true) {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd_, buffer, sizeof(buffer));
        if (nread > 0) {
          printf("recv(eventfd=%d): %s\n", fd_, buffer);
          send(fd_, buffer, strlen(buffer), 0);
        } else if (nread == -1 &&
                   errno == EINTR) { // Signal interrupt while reading
          continue;
        } else if (nread == -1 &&
                   ((errno == EAGAIN) ||
                    (errno == EWOULDBLOCK))) { // Finished reading
          break;
        } else if (nread == 0) {
          printf("Client(eventfd=%d) disconnected.\n", fd_);
          close(fd_);
          break;
        }
      }
    }
  } else if (revents_ & EPOLLOUT) { // Write event
  } else {                          // error
    printf("Client(eventfd=%d) error.\n", fd_);
    close(fd_);
  }
}
