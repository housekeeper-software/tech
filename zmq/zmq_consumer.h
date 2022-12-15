#ifndef MQ_ZMQ_ZMQ_CONSUMER_H_
#define MQ_ZMQ_ZMQ_CONSUMER_H_

#include <memory>
#include "build/build_config.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "base/threading/simple_thread.h"
#include "mq/zmq/zmq_helper.h"
#include "mq/zmq/mq.pb.h"

namespace zmq {

class ZMQConsumer
    : public base::DelegateSimpleThread::Delegate {
public:
 class Delegate {
 public:
  virtual ~Delegate() {}
  virtual void OnZMQMessageArrival(std::unique_ptr<protocol::MQMessage> message) = 0;
 protected:
  Delegate() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Delegate);
 };
 explicit ZMQConsumer(Delegate *delegate,
                      const scoped_refptr<ref_context_t> &context,
                      const std::string &url,
                      const std::string &identity={});

 virtual ~ZMQConsumer();

private:

 void Run() override;

 bool CreateSocket();

 void CloseSocket();

 bool ReadMessage(std::unique_ptr<protocol::MQMessage> *message);

 bool SendReply(const protocol::MQMessage *message);

 Delegate *delegate_;

 scoped_refptr<ref_context_t> context_;

 std::string url_;

 std::string identity_;

 std::unique_ptr<zmq::socket_t> zmq_socket_;

 bool keep_running_;

 std::unique_ptr<base::DelegateSimpleThread> thread_;

 DISALLOW_COPY_AND_ASSIGN(ZMQConsumer);
};

}  // namespace zmq

#endif  // MQ_ZMQ_ZMQ_CONSUMER_H_
