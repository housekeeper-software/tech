#ifndef MQ_ZMQ_ZMQ_PRODUCER_H_
#define MQ_ZMQ_ZMQ_PRODUCER_H_

#include <memory>
#include "build/build_config.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "base/threading/thread.h"
#include "mq/zmq/zmq_helper.h"
#include "mq/zmq/mq.pb.h"

namespace zmq {

class ZMQProducer {
public:
 explicit ZMQProducer(const scoped_refptr<ref_context_t> &context,
                      const std::string &url);

 virtual ~ZMQProducer();

 void Publish(std::unique_ptr<protocol::MQMessage> message);

private:
 void OnStart();

 void OnStop();

 void DoConnect();

 void SendFromCache();

 bool CreateSocket();

 void CloseSocket();

 bool Send(protocol::MQMessage *message);

 std::string url_;

 scoped_refptr<ref_context_t> context_;

 std::unique_ptr<zmq::socket_t> zmq_socket_;

 std::queue<std::unique_ptr<protocol::MQMessage>> message_list_;

 std::unique_ptr<base::Thread> thread_;

 base::WeakPtr<ZMQProducer> weak_ptr_;

 base::WeakPtrFactory<ZMQProducer> weak_factory_;

 DISALLOW_COPY_AND_ASSIGN(ZMQProducer);
};

}  // namespace zmq

#endif  // MQ_ZMQ_ZMQ_PRODUCER_H_
