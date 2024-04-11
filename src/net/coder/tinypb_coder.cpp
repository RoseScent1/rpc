#include "tinypb_coder.h"
#include "log.h"
#include "tinypb_protocol.h"
#include "util.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <netinet/in.h>
#include <string>

namespace rocket {

TinyPBCoder::~TinyPBCoder() {
  // RPC_INFO_LOG("~TinyPBCoder");
}

void TinyPBCoder::EnCode(std::vector<AbstractProtocol::s_ptr> &message,
                         TcpBuffer::s_ptr &out_buffer) {
  for (auto &i : message) {
    std::shared_ptr<TinyPBProtocol> msg =
        std::dynamic_pointer_cast<TinyPBProtocol>(i);
    int len = 0;
    const char *buffer = encodeTinyPB(msg, len);

    if (buffer != nullptr && len != 0) {
      out_buffer->WriteToBuffer(buffer, len);
    }

    if (buffer) {
      delete[] buffer;
    }
  }
}
void TinyPBCoder::Decode(std::vector<AbstractProtocol::s_ptr> &out_message,
                         TcpBuffer::s_ptr &in_buffer) {
  // 遍历out_buffer,找到PB_start,找到包长度
  while (in_buffer->ReadIndex() < in_buffer->WriteIndex()) {
    // 遍历 out_buffer，找到 s_start
    // 解析整包长度，然后找到 s_end;
    std::string tmp = in_buffer->buffer_;
    int start_index = in_buffer->ReadIndex();
    int end_index{-1};

    int pk_len{0};
    bool parse_success{false};
    int i{0};
    /// TODO: 修改一下名字
    for (i = start_index; i < in_buffer->WriteIndex(); ++i) {
      // 如果是开始的字节序
      if (tmp[i] == TinyPBProtocol::PB_START) {
        // 进行转换
        if (i + 1 < in_buffer->WriteIndex()) {
          pk_len = getInt32FromNetByte(&tmp[i + 1]);
          RPC_DEBUG_LOG("parse success, pk_len = %d", pk_len);

          // 结束符的索引
          int j = i + pk_len - 1;
          if (j >= in_buffer->WriteIndex()) {
            continue;
          }
          // 如果是结束符，说明有可能解析成功了
          if (tmp[j] == TinyPBProtocol::PB_END) {
            start_index = i;
            end_index = j;
            parse_success = true;
            break;
          }
        }
      }
    }

    if (i >= in_buffer->WriteIndex()) {
      RPC_DEBUG_LOG("decode end, read all out_buffer data");
      return;
    }

    if (parse_success) {

      in_buffer->ModifyReadIndex(end_index - start_index + 1);
      auto message = std::make_shared<TinyPBProtocol>();
      message->pk_len_ = pk_len;

      int msg_id_index = start_index + sizeof(char) + sizeof(message->pk_len_);
      if (msg_id_index >= end_index) {
        message->parse_success_ = false;
        RPC_ERROR_LOG("parse error, msg_id_index[%d] >= end_index[%d]", msg_id_index,
                 end_index);
        continue;
      }
      message->msg_id_ = getInt32FromNetByte(&tmp[msg_id_index]);
      RPC_DEBUG_LOG("parse msg_id_=%d", message->msg_id_);

      int method_name_len_index = msg_id_index + sizeof(message->msg_id_);
      if (method_name_len_index >= end_index) {
        message->parse_success_ = false;
        RPC_ERROR_LOG("parse error, method_name_len_index[%d] >= end_index[%d]",
                 method_name_len_index, end_index);
        continue;
      }
      message->method_name_len_ =
          getInt32FromNetByte(&tmp[method_name_len_index]);

      int method_name_index =
          method_name_len_index + sizeof(message->method_name_len_);
      char method_name[512] = {0};
      if (message->method_name_len_ != 0) {
        memcpy(&method_name[0], &tmp[method_name_index],
               message->method_name_len_);
        message->method_name_ = std::string(method_name);
        RPC_DEBUG_LOG("parse method_name=%s", message->method_name_.c_str());
      }
      int err_code_index = method_name_index + message->method_name_len_;
      if (err_code_index >= end_index) {
        message->parse_success_ = false;
        RPC_ERROR_LOG("parse error, err_code_index[%d] >= end_index[%d]",
                 err_code_index, end_index);
        continue;
      }
      message->err_code_ = getInt32FromNetByte(&tmp[err_code_index]);

      int error_info_len_index = err_code_index + sizeof(message->err_code_);
      if (error_info_len_index >= end_index) {
        message->parse_success_ = false;
        RPC_ERROR_LOG("parse error, error_info_len_index[%d] >= end_index[%d]",
                 error_info_len_index, end_index);
        continue;
      }
      message->err_info_len_ = getInt32FromNetByte(&tmp[error_info_len_index]);

      int err_info_index =
          error_info_len_index + sizeof(message->err_info_len_);
      if (message->err_info_len_ != 0) {
        char error_info[512] = {0};
        memcpy(&error_info[0], &tmp[err_info_index], message->err_info_len_);
        message->err_info_ = std::string(error_info);
        RPC_DEBUG_LOG("parse error_info=%d", message->err_info_.c_str());
      }

      int pb_data_len = message->pk_len_ - message->method_name_len_ -
                        sizeof(message->msg_id_) - message->err_info_len_ - 2 -
                        20;

      int pd_data_index = err_info_index + message->err_info_len_;
      message->pb_data_ = std::string(&tmp[pd_data_index], pb_data_len);
      // 这里校验和去解析

      message->parse_success_ = true;
      out_message.push_back((message));
    }
  }
}

const char *TinyPBCoder::encodeTinyPB(TinyPBProtocol::s_ptr message, int &len) {

  int pk_len = 2 + 20 + sizeof(message->msg_id_) +
               message->method_name_.length() + message->err_info_.length() +
               message->pb_data_.length();

  char *buffer = new char[pk_len]();
  char *tmp = buffer;
  *tmp = TinyPBProtocol::PB_START;
  ++tmp;

  int32_t pk_len_net = htonl(pk_len);
  memcpy(tmp, &pk_len_net, sizeof(pk_len_net));
  tmp += sizeof(pk_len_net);

  uint32_t msg_id_ = message->msg_id_;
  int32_t msg_id_net = htonl(msg_id_);
  memcpy(tmp, &msg_id_net, sizeof(msg_id_net));
  tmp += sizeof(msg_id_net);

  int method_name_len = message->method_name_.length();
  int32_t method_name_len_net = htonl(method_name_len);
  memcpy(tmp, &method_name_len_net, sizeof(method_name_len_net));
  tmp += sizeof(method_name_len_net);

  if (!message->method_name_.empty()) {

    memcpy(tmp, &(message->method_name_[0]), method_name_len);
    tmp += method_name_len;
  }

  int err_len_net = htonl(message->err_code_);
  memcpy(tmp, &err_len_net, sizeof(err_len_net));
  tmp += sizeof(err_len_net);

  int err_info_len = message->err_info_.length();
  int32_t err_info_len_net = htonl(err_info_len);
  memcpy(tmp, &err_info_len_net, sizeof(err_info_len_net));
  tmp += sizeof(method_name_len_net);

  if (!message->err_info_.empty()) {
    memcpy(tmp, &(message->err_info_[0]), sizeof(err_info_len));
    tmp += err_info_len;
  }

  if (!message->pb_data_.empty()) {
    memcpy(tmp, &(message->pb_data_[0]), message->pb_data_.length());
    tmp += message->pb_data_.length();
  }
  int32_t check_sum_net = htonl(1);
  memcpy(tmp, &check_sum_net, sizeof(check_sum_net));
  tmp += sizeof(check_sum_net);

  *tmp = TinyPBProtocol::PB_END;

  message->pk_len_ = pk_len;
  message->method_name_len_ = method_name_len;
  message->err_info_len_ = err_info_len;
  message->parse_success_ = true;
  len = pk_len;
  RPC_DEBUG_LOG("msg_id = %d, encode message success, len = %d", message->msg_id_, pk_len);
  return buffer;
}
} // namespace rocket
