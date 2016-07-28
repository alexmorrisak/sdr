// sockman.hpp
// message declarations for communicating with socket manager sockman

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

