#include "mq/zmq/zmq_helper.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace zmq {
message_t::message_t() noexcept
    : msg_{0} {
  int rc = zmq_msg_init(&msg_);
  DCHECK(rc == 0);
}

message_t::message_t(size_t size)
    : msg_{0} {
  int rc = zmq_msg_init_size(&msg_, size);
  DCHECK(rc == 0);
}

message_t::message_t(const void *buf, size_t size)
    : msg_{0} {
  int rc = zmq_msg_init_size(&msg_, size);
  DCHECK(rc == 0);
  if (size > 0) {
    memcpy(data(), buf, size);
  }
}

message_t::message_t(const std::string &str)
    : message_t(str.data(), str.size()) {}

message_t::~message_t() {
  int rc = zmq_msg_close(&msg_);
  DCHECK(rc == 0);
}

void message_t::rebuild() {
  int rc = zmq_msg_close(&msg_);
  DCHECK(rc == 0);
  rc = zmq_msg_init(&msg_);
  DCHECK(rc == 0);
}

void message_t::rebuild(size_t size) {
  int rc = zmq_msg_close(&msg_);
  DCHECK(rc == 0);
  rc = zmq_msg_init_size(&msg_, size);
  DCHECK(rc == 0);
}

void message_t::rebuild(const void *buf, size_t size) {
  int rc = zmq_msg_close(&msg_);
  DCHECK(rc == 0);
  rc = zmq_msg_init_size(&msg_, size);
  DCHECK(rc == 0);
  memcpy(data(), buf, size);
}

void message_t::rebuild(const std::string &str) {
  rebuild(str.data(), str.size());
}

void message_t::copy(message_t &msg) {
  int rc = zmq_msg_copy(&msg_, msg.handle());
  DCHECK(rc == 0);
}

bool message_t::more() const noexcept {
  int rc = zmq_msg_more(const_cast<zmq_msg_t *>(&msg_));
  return rc != 0;
}

void *message_t::data() noexcept {
  return zmq_msg_data(&msg_);
}

const void *message_t::data() const noexcept {
  return zmq_msg_data(const_cast<zmq_msg_t *>(&msg_));
}

size_t message_t::size() const noexcept {
  return zmq_msg_size(const_cast<zmq_msg_t *>(&msg_));
}

bool message_t::empty() const noexcept {
  return size() == 0U;
}

bool message_t::operator==(const message_t &other) {
  const size_t my_size = size();
  return my_size == other.size() && 0 == memcmp(data(), other.data(), my_size);
}

bool message_t::operator!=(const message_t &other) {
  return !(*this == other);
}

std::string message_t::to_string() const {
  return std::string(static_cast<const char *>(data()), size());
}

zmq_msg_t *message_t::handle() noexcept {
  return &msg_;
}

const zmq_msg_t *message_t::handle() const noexcept {
  return &msg_;
}

context_t::context_t()
    : ptr_(zmq_ctx_new()) {
}

context_t::context_t(int io_threads, int max_sockets)
    : ptr_(zmq_ctx_new()) {
  int rc = zmq_ctx_set(ptr_, ZMQ_IO_THREADS, io_threads);
  DCHECK(rc == 0);

  rc = zmq_ctx_set(ptr_, ZMQ_MAX_SOCKETS, max_sockets);
  DCHECK(rc == 0);

  rc = zmq_ctx_set(ptr_, ZMQ_BLOCKY, 0);
  DCHECK(rc == 0);
}

context_t::~context_t() {
  close();
}

void context_t::close() {
  if (ptr_) {
    HANDLE_EINTR(zmq_ctx_term(ptr_));
    ptr_ = nullptr;
  }
}

void context_t::shutdown() {
  if (ptr_) {
    int rc = zmq_ctx_shutdown(ptr_);
    DCHECK(rc == 0);
  }
}

void *context_t::handle() {
  return ptr_;
}

socket_t::socket_t(void *handle, int type)
    : handle_(zmq_socket(handle, type)) {}

socket_t::~socket_t() {
  close();
}

int socket_t::bind(const std::string &addr) {
  return zmq_bind(handle_, addr.c_str());
}

int socket_t::unbind(const std::string &addr) {
  return zmq_unbind(handle_, addr.c_str());
}

int socket_t::connect(const std::string &addr) {
  return zmq_connect(handle_, addr.c_str());
}

int socket_t::disconnect(const std::string &addr) {
  return zmq_disconnect(handle_, addr.c_str());
}

bool socket_t::connected() const {
  return handle_ != nullptr;
}

int socket_t::send(const void *data, size_t size, int flags) {
  return HANDLE_EINTR(zmq_send(handle_, data, size, flags));
}

int socket_t::send(message_t *msg, int flags) {
  return HANDLE_EINTR(zmq_msg_send(msg->handle(), handle_, flags));
}

int socket_t::recv(void *buf, size_t size, int flags) {
  const int nbytes = HANDLE_EINTR(zmq_recv(handle_, buf, size, flags));
  if (nbytes >= 0) {
    DCHECK(size == static_cast<size_t>(nbytes));
    return (nbytes);
  }
  return nbytes;
}

int socket_t::recv(message_t *msg, int flags) {
  const int nbytes = HANDLE_EINTR(zmq_msg_recv(msg->handle(), handle_, flags));
  if (nbytes >= 0) {
    DCHECK(msg->size() == static_cast<size_t>(nbytes));
    return (nbytes);
  }
  return nbytes;
}

void *socket_t::handle() {
  return handle_;
}

const void *socket_t::handle() const {
  return handle_;
}

void socket_t::close() {
  if (!handle_)
    // already closed
    return;
  int rc = zmq_close(handle_);
  DCHECK(rc == 0);
  handle_ = nullptr;
}
}  // namespace server
