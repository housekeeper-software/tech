#ifndef MQ_ZMQ_ZMQ_HELPER_H_
#define MQ_ZMQ_ZMQ_HELPER_H_

#include <memory>
#include "build/build_config.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "base/memory/ref_counted.h"
#include "mq/zmq/mq.pb.h"
#include <zmq.h>

namespace zmq {

class message_t {
public:
 message_t() noexcept;

 explicit message_t(size_t size);

 message_t(const void *buf, size_t size);

 explicit message_t(const std::string &str);

 virtual ~message_t();

 void rebuild();

 void rebuild(size_t size);

 void rebuild(const void *buf, size_t size);

 void rebuild(const std::string &str);

 void copy(message_t &msg);

 bool more() const noexcept;

 void *data() noexcept;

 const void *data() const noexcept;

 size_t size() const noexcept;

 bool empty() const noexcept;

 bool operator==(const message_t &other);

 bool operator!=(const message_t &other);

 std::string to_string() const;

 zmq_msg_t *handle() noexcept;

 const zmq_msg_t *handle() const noexcept;
private:
 zmq_msg_t msg_;
 DISALLOW_COPY_AND_ASSIGN(message_t);
};

class context_t {
public:
 context_t();

 explicit context_t(int io_threads, int max_sockets = ZMQ_MAX_SOCKETS_DFLT);

 virtual ~context_t();

 void close();

 void shutdown();

 void *handle();

 operator void *() noexcept { return ptr_; }

 operator void const *() const noexcept { return ptr_; }

private:
 void *ptr_;
 DISALLOW_COPY_AND_ASSIGN(context_t);
};

class ref_context_t
    : public base::RefCountedThreadSafe<ref_context_t> {
public:
 explicit ref_context_t(int io_threads = 1, int max_sockets = ZMQ_MAX_SOCKETS_DFLT)
     : context_(new context_t(io_threads, max_sockets)) {}

 void *handle() { return context_ ? context_->handle() : nullptr; }

 context_t *context() { return context_.get(); }

private:
 virtual ~ref_context_t() {}

 std::unique_ptr<context_t> context_;

 friend class base::RefCountedThreadSafe<ref_context_t>;

 DISALLOW_COPY_AND_ASSIGN(ref_context_t);
};

class socket_t {
public:
 explicit socket_t(void *handle, int type);

 virtual ~socket_t();

 int bind(const std::string &addr);

 int unbind(const std::string &addr);

 int connect(const std::string &addr);

 int disconnect(const std::string &addr);

 bool connected() const;

 int send(const void *data, size_t size, int flags = 0);

 int send(message_t *msg, int flags = 0);

 int recv(void *buf, size_t size, int flags = 0);

 int recv(message_t *msg, int flags = 0);

 void *handle();

 const void *handle() const;

 void close();
private:
 void *handle_;
 DISALLOW_COPY_AND_ASSIGN(socket_t);
};

}  // namespace zmq

#endif  // MQ_ZMQ_ZMQ_HELPER_H_
