#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct MessageHeader {
    std::uint32_t type;
    std::uint32_t length;
};

struct QueueMeta {
    std::atomic<std::size_t> head;
    std::atomic<std::size_t> tail;
    std::size_t capacity;
    std::uint32_t protocol_version;
};

class ProducerNode {
public:
    ProducerNode(const std::string& shm_name, std::size_t capacity);
    ~ProducerNode();

    ProducerNode(const ProducerNode&) = delete;
    ProducerNode& operator=(const ProducerNode&) = delete;

    bool Send(std::uint32_t type, std::string_view data);

private:
    QueueMeta* meta_{nullptr};
    std::uint8_t* buffer_;
    std::size_t capacity_{0};
    int shm_fd_{-1};
    std::size_t mapped_size_{0};
    std::string shm_name_;
};

class ConsumerNode {
public:
    explicit ConsumerNode(const std::string& shm_name);
    ~ConsumerNode();

    ConsumerNode(const ConsumerNode&) = delete;
    ConsumerNode& operator=(const ConsumerNode&) = delete;

    bool Receive(std::uint32_t desired_type, std::vector<std::uint8_t>& out);

private:
    QueueMeta* meta_{nullptr};
    std::uint8_t* buffer_;
    std::size_t capacity_{0};
    int shm_fd_{-1};
    std::size_t mapped_size_{0};
    std::string shm_name_;
};

