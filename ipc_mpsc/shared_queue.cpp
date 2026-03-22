#include "shared_queue.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr std::uint32_t kProtocolVersion = 1;

std::size_t ShmSize(std::size_t capacity) {
    return sizeof(QueueMeta) + capacity * sizeof(MessageSlot);
}

void* MapShm(int fd, std::size_t size) {
    void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        throw std::runtime_error("mmap failed: " +
                                 std::string(std::strerror(errno)));
    }
    return addr;
}

}

ProducerNode::ProducerNode(const std::string& shm_name,
                           std::size_t capacity)
    : capacity_(capacity), shm_name_(shm_name) {

    shm_unlink(shm_name.c_str());
    shm_fd_ = shm_open(shm_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd_ == -1) {
        throw std::runtime_error("shm_open(producer) failed: " +
                                 std::string(std::strerror(errno)));
    }

    mapped_size_ = ShmSize(capacity_);
    if (ftruncate(shm_fd_, static_cast<off_t>(mapped_size_)) == -1) {
        throw std::runtime_error("ftruncate failed: " +
                                 std::string(std::strerror(errno)));
    }

    void* addr = MapShm(shm_fd_, mapped_size_);
    meta_ = static_cast<QueueMeta*>(addr);
    slots_ = reinterpret_cast<MessageSlot*>(
                 static_cast<std::uint8_t*>(addr) + sizeof(QueueMeta));

    meta_->head.store(0, std::memory_order_relaxed);
    meta_->tail.store(0, std::memory_order_relaxed);
    meta_->capacity = capacity_;
    meta_->protocol_version = kProtocolVersion;

    for (std::size_t i = 0; i < capacity_; ++i) {
        slots_[i].state.store(0, std::memory_order_relaxed);
    }

    std::atomic_thread_fence(std::memory_order_release);
}

ProducerNode::~ProducerNode() {
    if (meta_) {
        munmap(meta_, mapped_size_);
    }
    if (shm_fd_ != -1) {
        close(shm_fd_);
    }
    if (!shm_name_.empty()) {
        shm_unlink(shm_name_.c_str());
    }
}

bool ProducerNode::Send(std::uint32_t type, std::string_view data) {
    if (data.size() > kMaxPayload) {
        throw std::runtime_error("payload too large");
    }

    while (true) {
        std::size_t tail = meta_->tail.load(std::memory_order_acquire);
        std::size_t head = meta_->head.load(std::memory_order_acquire);

        if (tail - head >= capacity_) {
            return false;
        }

        if (meta_->tail.compare_exchange_weak(
                tail, tail + 1, std::memory_order_acq_rel)) {
            std::size_t  idx  = tail % capacity_;
            MessageSlot& slot = slots_[idx];

            while (slot.state.load(std::memory_order_acquire) != 0) {
            }

            slot.header.type = type;
            slot.header.length = static_cast<std::uint32_t>(data.size());
            std::memcpy(slot.payload, data.data(), data.size());

            slot.state.store(1, std::memory_order_release);
            return true;
        }
    }
}

ConsumerNode::ConsumerNode(const std::string& shm_name)
    : shm_name_(shm_name) {

    shm_fd_ = shm_open(shm_name.c_str(), O_RDWR, 0666);
    if (shm_fd_ == -1) {
        throw std::runtime_error("shm_open(consumer) failed: " +
                                 std::string(std::strerror(errno)));
    }

    struct stat st{};
    if (fstat(shm_fd_, &st) == -1) {
        throw std::runtime_error("fstat failed");
    }
    mapped_size_ = static_cast<std::size_t>(st.st_size);

    void* addr = MapShm(shm_fd_, mapped_size_);
    meta_  = static_cast<QueueMeta*>(addr);
    slots_ = reinterpret_cast<MessageSlot*>(
                 static_cast<std::uint8_t*>(addr) + sizeof(QueueMeta));

    std::atomic_thread_fence(std::memory_order_acquire);

    if (meta_->protocol_version != kProtocolVersion) {
        throw std::runtime_error(
            "protocol version mismatch: expected " +
            std::to_string(kProtocolVersion) + ", got " +
            std::to_string(meta_->protocol_version));
    }
    capacity_ = meta_->capacity;
}

ConsumerNode::~ConsumerNode() {
    if (meta_) {
        munmap(meta_, mapped_size_);
    }
    if (shm_fd_ != -1) {
        close(shm_fd_);
    }
}

bool ConsumerNode::Receive(std::uint32_t desired_type,
                           std::vector<std::uint8_t>& out) {
    while (true) {
        std::size_t head = meta_->head.load(std::memory_order_relaxed);
        std::size_t tail = meta_->tail.load(std::memory_order_acquire);

        if (head >= tail) {
            return false;
        }

        std::size_t idx = head % capacity_;
        MessageSlot& slot = slots_[idx];

        while (slot.state.load(std::memory_order_acquire) != 1) {
        }

        bool matched = (slot.header.type == desired_type);
        if (matched) {
            out.assign(slot.payload,
                       slot.payload + slot.header.length);
        }

        slot.state.store(0, std::memory_order_release);
        meta_->head.store(head + 1, std::memory_order_release);

        if (matched) {
            return true;
        }
    }
}