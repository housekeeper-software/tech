#ifndef MQ_ZMQ_ZMQ_PROTOCOL_H_
#define MQ_ZMQ_ZMQ_PROTOCOL_H_

#include <memory>
#include "build/build_config.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "mq/zmq/mq.pb.h"
#include <zmq.h>

namespace zmq {
enum ZMQProtocol {
 ZMQ_TYPE_MESSAGE,
 ZMQ_TYPE_BROADCAST,
 ZMQ_TYPE_KILL,
 ZMQ_TYPE_OFFLINE
};
}  // namespace zmq

#endif  // MQ_ZMQ_ZMQ_PROTOCOL_H_
