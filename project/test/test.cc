// Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutongjia@163.com)
//

#include <iostream>

template <typename... Args> void ArgsSize(Args... args) {
  std::cout << sizeof...(args) << std::endl;
}

int main() {
  ArgsSize(1, "string", 3.14);
  ArgsSize();
  return 0;
}
