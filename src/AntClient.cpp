//
// Created by sunnysab on 2021/6/23.
//

#include <stdexcept>
#include <filesystem.h>
#include <string.h>
#include <protocol/RequestPayload.h>
#include <protocol/RecvResponse.h>
#include <protocol/DataPayload.h>
#include "AntClient.h"


AntClient::AntClient(const std::string &path_, const std::string &path_dst_) {
    this->in_ = new std::ifstream;
    this->file_path_ = path_;
    this->in_->open(path_, std::ios::in | std::ios::binary);

    if (!this->in_->is_open()) {
        std::logic_error ex( (std::string) "Could not open this file: " + strerror(errno) );
        throw std::exception(ex);
    }
    this->buffer_.open(this->in_);

    size_t rpos = path_dst_.find_last_of("\\/");
    if (rpos == path_dst_.size() - 1) {
        rpos = path_.find_last_of("\\/");
        if (rpos == std::string::npos) {
            this->file_path_dst_ = path_dst_ + path_;
        } else {
            this->file_path_dst_ = path_dst_ + path_.substr(rpos + 1);
        }
    } else {
        this->file_path_dst_ = path_dst_;
    }
    printf("src[%s] dst[%s]\n\n", path_.c_str(), this->file_path_dst_.c_str());
}

void AntClient::connect(const std::string &host, const port_t port) {
    this->socket_.connect(host, port);
}

bool AntClient::try_send() {
    Frame request_frame(0, 0, FrameType::SendRequest);
    RequestPayload *payload = this->prepare_send_request();
    request_frame.put(payload);
    auto buffer = request_frame.serialize();
    delete payload;
    payload = nullptr;

    this->socket_.send(buffer);
    auto received_packet = this->socket_.recvfrom();

    Frame response = Frame::deserialize(received_packet.payload);
    auto *srv_resp = dynamic_cast<RecvResponse *>(response.payload);
    auto srv_ack = srv_resp->response;
    delete srv_resp;
    srv_resp = nullptr;

    return srv_ack;
}

void AntClient::close() {
    this->socket_.close();
}

std::string AntClient::parse_file_name(const std::string &s) {
    auto pos_slash = s.rfind('/');
    auto pos_backslash = s.rfind('\\');
    if (pos_slash == static_cast<size_t>(-1))
        pos_slash = 0;
    else
        pos_slash++;

    if (pos_backslash == static_cast<size_t>(-1))
        pos_backslash = 0;
    else
        pos_slash++;

    auto file_name_start = pos_slash > pos_backslash ? pos_slash : pos_backslash;
    return s.substr(file_name_start);
}

RequestPayload *AntClient::prepare_send_request() {
    std::error_code err_code;
    this->file_size_ = fs::file_size(this->file_path_, err_code);
    if (err_code.value() != 0){
        std::logic_error ex(err_code.message());
        throw std::exception(ex);
    }

    auto *payload = new RequestPayload();
    payload->client_name = "client";
    //payload->file_name = parse_file_name(this->file_path_);
    payload->file_name = this->file_path_dst_;
    payload->file_size = this->file_size_;
    return payload;
}

void AntClient::send(const std::function<bool(TransferProcess)> &callback) {
    auto &status = this->status_;

    status.completed_size = 0;
    status.total_size = this->file_size_;

    while (status.completed_size < status.total_size) {
        // Read from buffer.
        auto disk_buffer = this->buffer_.read(1024);
        auto *payload = new DataPayload();
        payload->content = disk_buffer;

        Frame frame(0, 0, FrameType::Data);;
        frame.put(payload);
        auto net_buffer = frame.serialize();

        this->socket_.send(net_buffer);
        status.completed_size += disk_buffer.size();

        if (callback && !callback(status)) {
            std::logic_error ex("Process broken because user cancelled.");
            throw std::exception(ex);
        } else if(!callback) {
            printf("%zu/%zu\n", status.completed_size, status.total_size);
        }

        auto received_packet = this->socket_.recvfrom();
        Frame response = Frame::deserialize(received_packet.payload);
        auto *srv_resp = dynamic_cast<RecvResponse *>(response.payload);
        auto srv_ack = srv_resp->response;
        delete srv_resp;
        srv_resp = nullptr;
        if (!srv_ack) {
            printf("recv response fail\n");
        }
    }
}
