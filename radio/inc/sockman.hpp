// sockman.hpp
// message declarations for communicating with socket manager sockman
#ifndef SOCKMAN_H
#define SOCKMAN_H

enum sockManMsgType {
  ACK,
  REGISTER,
  UNREGISTER,
  SUBSCRIBE,
  UNSUBSCRIBE,
  NOTIFY
};

struct sockManMsg {
  sockManMsgType type;
  size_t length;
};

struct notifyMsg {
  size_t size;
  int location;
  int id;
};

#endif


