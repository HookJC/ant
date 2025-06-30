//
// Created by sunnysab on 2021/6/23.
//

#include <stdexcept>
#include <filesystem.h>
#include <string.h>
#include <protocol/RequestPayload.h>
#include <protocol/RecvResponse.h>
#include <protocol/DataPayload.h>
#include "AntServer.h"


AntServer::AntServer() {
    out_    = nullptr;
    buffer_ = nullptr;
}

void AntServer::listen(const std::string &host, const port_t port) {
    this->socket_.bind(host, port);
}

RequestPayload AntServer::wait() {
    auto request_bin = this->socket_.recvfrom();
    this->socket_.connect(request_bin.host, request_bin.port);

    auto request = Frame::deserialize(request_bin.payload);
    this->file_size_ = reinterpret_cast<RequestPayload *>(request.payload)->file_size;
    return *dynamic_cast<RequestPayload *>(request.payload);
}

bool AntServer::fopen(const std::string &path_) {
    this->out_ = new std::ofstream;
    this->file_path_ = path_;
    this->out_->open(path_, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!this->out_->is_open()) {
        printf("Could not open this file: %s\n", strerror(errno));
        return false;
    }

    // this->file_size_ = fs::file_size(path_);
    this->buffer_ = new WriteBuffer(this->out_);
    return true;
}

void AntServer::fclose(){
    if (!out_) return;
    delete buffer_;
    buffer_ = nullptr;
    out_->close();
    out_ = nullptr;
}

void AntServer::close() {
    this->socket_.close();
    // TODO: Memory leak. this->buffer_ is not free now.
}

void AntServer::write(const std::function<bool(TransferProcess)> &callback) {
    auto &status = this->status_;

    status.completed_size = 0;
    status.total_size = this->file_size_;

    while (status.completed_size < status.total_size) {
        // Read from socket
        auto buffer = this->socket_.recvfrom();
        auto frame = Frame::deserialize(buffer.payload);
        auto content = ((DataPayload *) frame.payload)->content;

        this->buffer_->write(content);
        status.completed_size += content.size();

        if (callback && !callback(status)) {
            std::logic_error ex("Process broken because us er cancelled.");
            throw std::exception(ex);
        } else if(!callback) {
            printf("%zu/%zu\n", status.completed_size, status.total_size);
        }

        this->accept();
    }
    this->buffer_->flush();
}

void AntServer::accept() {
    Frame data_frame(0, 0, FrameType::RecvResponse);
    auto *payload = new RecvResponse();

    payload->response = true;
    data_frame.put(payload);

    auto buffer = data_frame.serialize();
    this->socket_.send(buffer);
}

void AntServer::decline() {
    Frame data_frame(0, 0, FrameType::RecvResponse);
    auto *payload = new RecvResponse();

    payload->response = false;
    data_frame.put(payload);

    auto buffer = data_frame.serialize();
    this->socket_.send(buffer);
}
