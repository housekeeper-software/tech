#include "mq/zmq/zmq_producer.h"
#include "base/bind.h"
#include "base/posix/eintr_wrapper.h"

namespace zmq {

ZMQProducer::ZMQProducer(const scoped_refptr<ref_context_t> &context,
                         const std::string &url)
    : context_(context),
      url_(url),
      thread_(new base::Thread("ZMQProducer")),
      weak_factory_(this) {
  weak_ptr_ = weak_factory_.GetWeakPtr();
  base::Thread::Options option;
  option.message_loop_type = base::MessageLoop::Type::TYPE_IO;
  thread_->StartWithOptions(option);
  thread_->task_runner()->PostTask(FROM_HERE,
                                   base::Bind(&ZMQProducer::OnStart,
                                              weak_factory_.GetWeakPtr()));
}

ZMQProducer::~ZMQProducer() {
  if (thread_) {
    thread_->task_runner()->PostTask(FROM_HERE,
                                     base::Bind(&ZMQProducer::OnStop,
                                                weak_factory_.GetWeakPtr()));
    thread_->Stop();
    thread_.reset();
  }
}

void ZMQProducer::OnStart() {
  DoConnect();
}

void ZMQProducer::OnStop() {
  weak_factory_.InvalidateWeakPtrs();
  CloseSocket();
}

void ZMQProducer::Publish(std::unique_ptr<protocol::MQMessage> message) {
  if (!thread_->task_runner()->BelongsToCurrentThread()) {
    thread_->task_runner()->PostTask(FROM_HERE,
                                     base::Bind(&ZMQProducer::Publish,
                                                weak_ptr_,
                                                base::Passed(&message)));
  } else {
    message_list_.emplace(std::move(message));
    SendFromCache();
  }
}

void ZMQProducer::SendFromCache() {
  if (!zmq_socket_) return;

  while (!message_list_.empty()) {
    protocol::MQMessage *message = message_list_.front().get();
    if (!Send(message)) {
      CloseSocket();
      thread_->task_runner()->PostTask(FROM_HERE,
                                       base::Bind(&ZMQProducer::DoConnect,
                                                  weak_factory_.GetWeakPtr()));
      break;
    }
    message_list_.pop();
  }
}

bool ZMQProducer::CreateSocket() {
  CloseSocket();
  zmq_socket_ = std::make_unique<zmq::socket_t>(context_->handle(), ZMQ_REQ);
  if (!zmq_socket_->handle()) {
    LOG(ERROR) << "producer create socket failed: " << zmq_strerror(zmq_errno());
    return false;
  }
  int opt = 0;
  int rc = zmq_setsockopt(zmq_socket_->handle(), ZMQ_LINGER, &opt, sizeof(opt));
  DCHECK(rc == 0);
  if (zmq_socket_->connect(url_)) {
    LOG(ERROR) << "producer connect: " << url_ << " failed: " << zmq_strerror(zmq_errno());
    return false;
  }
  LOG(INFO) << "producer connect: " << url_ << " success";
  return true;
}

void ZMQProducer::CloseSocket() {
  zmq_socket_.reset();
}

void ZMQProducer::DoConnect() {
  if (!CreateSocket()) {
    CloseSocket();
    thread_->task_runner()->PostDelayedTask(FROM_HERE,
                                            base::Bind(&ZMQProducer::DoConnect,
                                                       weak_factory_.GetWeakPtr()),
                                            base::TimeDelta::FromMilliseconds(100));
  } else {
    SendFromCache();
  }
}

bool ZMQProducer::Send(protocol::MQMessage *message) {
  std::string str;
  message->SerializeToString(&str);
  int rc = zmq_socket_->send(str.data(), str.size());
  if (rc < 0) {
    LOG(ERROR) << "producer send error: " << zmq_strerror(zmq_errno());
    return false;
  }

  zmq_pollitem_t items[] = {
      {zmq_socket_->handle(), 0, ZMQ_POLLIN, 0}
  };

  rc = HANDLE_EINTR(zmq_poll(items, 1, 500));
  if (rc < 0) {
    LOG(ERROR) << "producer poll error: " << zmq_strerror(zmq_errno());
    return false;
  }
  if (items[0].revents & ZMQ_POLLIN) {
    message_t msg;
    rc = zmq_socket_->recv(&msg, 0);
    if (rc < 0) {
      LOG(ERROR) << "producer poll error: " << zmq_strerror(zmq_errno());
      return false;
    }
    std::unique_ptr<protocol::MQMessage> reply(new protocol::MQMessage());
    if (reply->ParseFromArray(msg.data(), msg.size())) {
      return reply->ack();
    }
  } else {
    LOG(ERROR) << "producer poll timeout";
  }
  return false;
}
}  // namespace zmq
