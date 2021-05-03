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

#include "JsonObject.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <string>


// s -> c  c -> s message {header, body} // header length

struct Header {
  int bodySize;
  int type;
};

// 消息类别
enum MessageType{
  MT_BIND_NAME = 0, // {"name" : "abc"}
  MT_CHAT_INFO = 1, // {"information" : "what i say"}
  MT_ROOM_INFO = 2, // {"name" : "abc", "information" : "what i say"}
};

class chat_message {
public:
  enum { header_length = sizeof(Header) }; // 包头
  enum { max_body_length = 512 };          // 包体

  chat_message() {}

  const char *data() const { return data_; }

  char *data() { return data_; }

  std::size_t length() const { return header_length + m_header.bodySize; }

  const char *body() const { return data_ + header_length; }

  char *body() { return data_ + header_length; }

  int type() const { return m_header.type; }

  std::size_t body_length() const { return m_header.bodySize; }

  // 设置消息格式
  void setMessage(int messageType, const void *buffer, size_t bufferSize) {
    assert(bufferSize <= max_body_length);
    m_header.bodySize = bufferSize;
    m_header.type = messageType;
    std::memcpy(body(), buffer, bufferSize);
    std::memcpy(data(), &m_header, sizeof(m_header));
  }

  bool setMessage(int messageType, const std::string &buffer) {
    setMessage(messageType, buffer.data(), buffer.size());
    return true;
  }

  //// 包体长度
  //void body_length(std::size_t new_length) {
  //  body_length_ = new_length;
  //  if (body_length_ > max_body_length)
  //    body_length_ = max_body_length;
  //}

  // 对包头进行解码
  bool decode_header() {
    std::memcpy(&m_header, data(), header_length);
    std::cout << m_header.type << " " << m_header.bodySize << std::endl;
    if (m_header.bodySize > max_body_length) {
      std::cout << "body size " << m_header.bodySize << " " << m_header.type
                << std::endl;
      return false;
    }
    return true;
    // char header[header_length + 1] = "";    // 注意要加一位存放字符串终止字符
    // std::strncat(header, data_, header_length);
    // body_length_ = std::atoi(header);
    // if (body_length_ > max_body_length) {
    //  body_length_ = 0;
    //  return false;
    //}
    // return true;
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

//template <typename T> std::string serialize(const T &obj) {
//  std::stringstream ss; // 字符串流
//  boost::archive::text_oarchive oa(ss);
//  oa &obj; // 将obj序列化输入到oa（即ss字符串）中去
//  return ss.str();
//}
//
//bool parseMessage(const std::string &input, int *type, std::string &outBuffer) {
//  auto pos = input.find_first_of(" ");
//  if (pos == std::string::npos)
//    return false;
//  if (pos == 0)
//    return false;
//  // "BindName ok" -> substr -> BindName
//  auto command = input.substr(0, pos);
//  if (command == "BindName") {
//    // try to bind name
//    std::string name = input.substr(pos + 1);
//    if (name.size() > 32)
//      return false;
//    if (type)
//      *type = MT_BIND_NAME;
//    outBuffer = serialize(SBindName(std::move(name)));
//    return true;
//  } else if (command == "Chat") {
//    // try to chat
//    std::string chat = input.substr(pos + 1);
//    if (chat.size() > 256)
//      return false;
//    outBuffer = serialize(SChatInfo(std::move(chat)));
//    if (type)
//      *type = MT_CHAT_INFO;
//    return true;
//  }
//  return false;
//}

#endif // CHAT_MESSAGE_HPP
