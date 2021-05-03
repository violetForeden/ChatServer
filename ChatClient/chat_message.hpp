//
// chat_message.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#pragma warning(disable : 4996)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <iostream>
#include <string>

// s -> c  c -> s message {header, body} // header length

struct Header {
  int bodySize;
  int type;
};

class chat_message {
 public:
  enum { header_length = sizeof(Header) };       // 包头
  enum { max_body_length = 512 };   // 包体 序列化下消息可以动态增长，可不用限制最大值

  chat_message() {}

  const char* data() const { return data_; }

  char* data() { return data_; }

  std::size_t length() const { return header_length + m_header.bodySize; }

  const char* body() const { return data_ + header_length; }

  char* body() { return data_ + header_length; }

  int type() const { return m_header.type; }

  std::size_t body_length() const { return m_header.bodySize; }

  // 设置消息格式
  void setMessage(int messageType, const void* buffer, size_t bufferSize) {
    assert(bufferSize <= max_body_length);
    m_header.bodySize = bufferSize;
    m_header.type = messageType;
    std::memcpy(body(), buffer, bufferSize);
    std::memcpy(data(), &m_header, sizeof(m_header));
  }

  bool setMessage(int messageType, const std::string &buffer) {
    setMessage(messageType, buffer.data(), buffer.size());
  }

  //// 包体长度
  //void body_length(std::size_t new_length) {
  //  m_header.bodySize = new_length;
  //  if (m_header.bodySize > max_body_length) 
  //      m_header.bodySize = max_body_length;
  //}

  // 对包头进行解码
  bool decode_header() {
    std::memcpy(&m_header, data(), header_length);
    if (m_header.bodySize > max_body_length) {
      std::cout << "body size " << m_header.bodySize << " " << m_header.type
                << std::endl;
      return false;
    }
    return true;
    //char header[header_length + 1] = "";    // 注意要加一位存放字符串终止字符
    //std::strncat(header, data_, header_length);
    //body_length_ = std::atoi(header);
    //if (body_length_ > max_body_length) {
    //  body_length_ = 0;
    //  return false;
    //}
    //return true;
  }

  //// 向包头填充字节
  //void encode_header() {
  //  char header[header_length + 1] = "";
  //  std::sprintf(header, "%4d", static_cast<int>(body_length_));
  //  std::memcpy(data_, header, header_length);
  //}

 private:
  char data_[header_length + max_body_length];
  //std::size_t body_length_;
  Header m_header;
};

#endif  // CHAT_MESSAGE_HPP
