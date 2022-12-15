#include "mq/zmq/zmq_consumer.h"
#include "base/posix/eintr_wrapper.h"

namespace zmq {
ZMQConsumer::ZMQConsumer(Delegate *delegate,
                         const scoped_refptr<ref_context_t> &context,
                         const std::string &url,
                         const std::string &identity)
    : delegate_(delegate),
      context_(context),
      url_(url),
      identity_(identity),
      keep_running_(true),
      thread_(new base::DelegateSimpleThread(this, "ZMQConsumer")) {
  thread_->Start();
}

ZMQConsumer::~ZMQConsumer() {
  if (thread_) {
    keep_running_ = false;
    thread_->Join();
    thread_.reset();
  }
}

void ZMQConsumer::Run() {
  int rc = 0;
  while (keep_running_) {
    if (!CreateSocket()) {
      CloseSocket();
      base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(100));
      continue;
    }

    while (keep_running_) {
      zmq_pollitem_t items[] = {
          {zmq_socket_->handle(), 0, ZMQ_POLLIN, 0}
      };

      rc = HANDLE_EINTR(zmq_poll(items, 1, 500));
      if (rc < 0) {
        LOG(ERROR) << "consumer poll error: " << zmq_strerror(zmq_errno());
        break;
      }
      if (items[0].revents & ZMQ_POLLIN) {
        std::unique_ptr<protocol::MQMessage> new_message;
        if (!ReadMessage(&new_message)) break;
        if (!SendReply(new_message.get())) break;
        delegate_->OnZMQMessageArrival(std::move(new_message));
      }
    }
  }
  CloseSocket();
}

bool ZMQConsumer::CreateSocket() {
  CloseSocket();
  zmq_socket_ = std::make_unique<zmq::socket_t>(context_->handle(), ZMQ_REP);
  if (!zmq_socket_->handle()) {
    LOG(ERROR) << "consumer create socket failed: " << zmq_strerror(zmq_errno());
    return false;
  }
  int opt = 0;
  int rc = zmq_setsockopt(zmq_socket_->handle(), ZMQ_LINGER, &opt, sizeof(opt));
  DCHECK(rc == 0);
  if (!identity_.empty()) {
    rc = HANDLE_EINTR(zmq_setsockopt(zmq_socket_->handle(), ZMQ_ROUTING_ID, identity_.data(), identity_.size()));
    DCHECK(rc == 0);
  }
  if (zmq_socket_->bind(url_)) {
    LOG(ERROR) << "consumer bind: " << url_ << " failed: " << zmq_strerror(zmq_errno());
    return false;
  }
  LOG(INFO) << "consumer bind: " << url_ << " success";
  return true;
}

void ZMQConsumer::CloseSocket() {
  zmq_socket_.reset();
}

bool ZMQConsumer::ReadMessage(std::unique_ptr<protocol::MQMessage> *message) {
  message_t msg;
  int rc = zmq_socket_->recv(&msg, 0);
  if (rc < 0) {
    LOG(ERROR) << "consumer recv error: " << zmq_strerror(zmq_errno());
    return false;
  }
  std::unique_ptr<protocol::MQMessage> new_message(new protocol::MQMessage());
  if (new_message->ParseFromArray(msg.data(), msg.size())) {
    *message = std::move(new_message);
    return true;
  }
  return false;
}

bool ZMQConsumer::SendReply(const protocol::MQMessage *message) {
  std::unique_ptr<protocol::MQMessage> reply(new protocol::MQMessage());
  reply->set_id(message->id());
  reply->set_type(message->type());
  reply->set_extension(message->extension());
  reply->set_ack(true);

  std::string data;
  reply->SerializeToString(&data);

  int rc = zmq_socket_->send(data.data(), data.size());
  if (rc < 0) {
    LOG(ERROR) << "consumer send reply error: " << zmq_strerror(zmq_errno());
    return false;
  }
  return true;
}
}  // namespace server
