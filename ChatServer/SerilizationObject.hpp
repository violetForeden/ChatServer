#pragma once

/*
        C++����Ϣ���л�Э�� ʹ��boost Serialization��
*/
#include "chat_message.hpp"

#include <boost/archive/text_iarchive.hpp> // boost���д������л��Ŀ�
#include <boost/archive/text_oarchive.hpp>


//enum { MT_BIND_NAME = 0, MT_CHAT_INFO = 1, MT_ROOM_INFO = 2};


// ���л��࣬���ݿɴ���
class SBindName {
private:
  friend class boost::serialization::access;    // ���������һ�У�ʹ�����serialize��������ͨ��boost��ʹ��������
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar &m_bindName; // archive��������&��������书��������>>���<<������Ľ����
    //ar << m_bindName;
    //ar >> m_bindName;
  }
  std::string m_bindName;

public:
  SBindName(std::string name) : m_bindName(std::move(name)) {}
  SBindName() {}
  const std::string &bindName() const { return m_bindName; }
};

class SChatInfo {
private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &m_chatInformation;
  } 
  std::string m_chatInformation;

public:
  SChatInfo(std::string info) : m_chatInformation(std::move(info)) {}
  SChatInfo() {}
  const std::string &chatInformation() const { return m_chatInformation; }
};

class SRoomInfo {
public:
  SRoomInfo() {}
  SRoomInfo(std::string name, std::string info)
      : m_bind(std::move(name)), m_chat(std::move(info)) {}
  const std::string &name() const { return m_bind.bindName(); }
  const std::string &information() const { return m_chat.chatInformation(); }
  
private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &m_bind;
    ar &m_chat;
  }
  SBindName m_bind;
  SChatInfo m_chat;
};

//template <typename T> std::string serialize(const T& obj) {
//  std::stringstream ss; // �ַ�����
//  boost::archive::text_oarchive oa(ss);
//  oa &obj;  // ��obj���л����뵽oa����ss�ַ�������ȥ
//  return ss.str();
//}
//
//bool parseMessage(const std::string& input, int* type, std::string& outBuffer) {
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