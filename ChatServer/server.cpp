/*
        一个简单的聊天服务器
*/
#include "chat_message.hpp"
#include "SerilizationObject.hpp"
#include "JsonObject.h"

#include <boost/asio.hpp>

#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

#include <cstdlib>

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

using chat_message_queue = std::deque<chat_message>;

//----------------------------------------------------------------------
// 基类，抽象类
class chat_participant {
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message &msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

//----------------------------------------------------------------------

class chat_room {
public:
  // 加入聊天室
  void join(chat_participant_ptr participant) {
    participants_.insert(participant);
    //std::cout << "插入聊天室！\n";
    for (const auto &msg : recent_msgs_)
      participant->deliver(msg);
  }

  // 离开聊天室
  void leave(chat_participant_ptr participant) {
    participants_.erase(participant);
  }

  // 分发消息
  void deliver(const chat_message &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto &participant :
         participants_) // 智能指针拷贝消耗大概是普通指针的20倍左右
      participant->deliver(msg);
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 }; // 包存最近100条消息
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------
// 客户端会话类
class chat_session : public chat_participant,
                     public std::enable_shared_from_this<chat_session> {
public:
  chat_session(tcp::socket socket, chat_room &room)
      : socket_(std::move(socket)), room_(room) {}

  void start() {
    room_.join(shared_from_this()); // 向聊天室加入一个客户端
    do_read_header();
  }

  void deliver(const chat_message &msg) {
    // 第一次是false
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
      // 第一次
      do_write();
    }
  }

private:
  void do_read_header() {
    auto self(shared_from_this()); // 防止指针被析构
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && read_msg_.decode_header()) {
            //std::cout << "读取包头成功！\n";
            do_read_body();
          } else {
            //std::cout << "error: " << ec << std::endl;
            std::cout << "Player leave the room\n";
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body() {
    auto self(shared_from_this());
    //std::cout << "进入do read body！" << std::endl;
    boost::asio::async_read(
        socket_, 
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          //std::cout << "进入处理包体回调！" << std::endl;
          if (!ec) {
            //room_.deliver(read_msg_);
            //std::cout << "读取包体成功！" << std::endl;
            handleMessage();
            do_read_header();
          } else {
            std::cout << "error: " << ec << std::endl;
            room_.leave(shared_from_this());
          }
        });
  }
  // 模板对象
  template <typename T> T toObject() {
    T obj;
    std::stringstream ss(std::string(read_msg_.body(),
                         read_msg_.body() + read_msg_.body_length()));
    boost::archive::text_iarchive oa(ss);
    oa &obj;
    return obj;
  }

  ptree toPtree() {
    ptree obj;
    std::stringstream ss(std::string(
        read_msg_.body(), read_msg_.body() + read_msg_.body_length()));
    boost::property_tree::read_json(ss, obj);
    return obj;
  } 

  // 处理消息 （JSON）
  void handleMessage() {
    if (read_msg_.type() == MT_BIND_NAME) {
     
      auto nameTree = toPtree();
      m_name = nameTree.get<std::string>("name");
    } else if (read_msg_.type() == MT_CHAT_INFO) {
      
      auto chat = toPtree();
      m_chatInformation = chat.get<std::string>("information");

      auto rinfo = buildRoomInfo();
      chat_message msg;
      msg.setMessage(MT_ROOM_INFO, rinfo);
      room_.deliver(msg);

    } else {
      // not valid msg do nothing
    }
  }

  //// 处理消息 （C++类序列化版）
  //void handleMessage() { 
  //  if (read_msg_.type() == MT_BIND_NAME) {
  //    /*SBindName bindName;
  //    std::stringstream ss(read_msg_.body());
  //    boost::archive::text_iarchive oa(ss);
  //    oa &bindName;*/
  //    
  //    auto bindName = toObject<SBindName>();
  //    m_name = bindName.bindName();
  //  } else if (read_msg_.type() == MT_CHAT_INFO) {
  //    /*SChatInfo chat;
  //    std::stringstream ss(std::string(
  //        read_msg_.body(), read_msg_.body() + read_msg_.body_length()));
  //    boost::archive::text_iarchive oa(ss);
  //    oa &chat;*/
  //    auto chat = toObject<SChatInfo>();
  //    m_chatInformation = chat.chatInformation();

  //    auto rinfo = buildRoomInfo();
  //    chat_message msg;
  //    msg.setMessage(MT_ROOM_INFO, rinfo);
  //    room_.deliver(msg);
  //    
  //  } else {
  //    // not valid msg do nothing
  //  }
  //}

  void do_write() {
    auto self(shared_from_this());
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_msgs_.front().data(),
                            write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
              do_write();
            }
          } else {
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room &room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  std::string m_name;
  std::string m_chatInformation;
  std::string buildRoomInfo() const {
    ptree tree;
    tree.put("name", m_name);
    tree.put("information", m_chatInformation);
    return ptreeToJsonString(tree);

    /*SRoomInfo roomInfo(m_name, m_chatInformation);
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa &roomInfo;
    return ss.str();*/
  }
};

//----------------------------------------------------------------------
// 服务器类
class chat_server {
public:
  chat_server(boost::asio::io_context &io_context,
              const tcp::endpoint &endpoint)
      : acceptor_(io_context, endpoint) {
    do_accept();
  }

private:
  void do_accept() {
    acceptor_.async_accept([this](boost::system::error_code ec,
                                  tcp::socket socket) {
      if (!ec) {
        auto session = std::make_shared<chat_session>(std::move(socket), room_);
        session->start();
        std::cout << "客户端建立连接" << std::endl;
      }

      do_accept();
    });
  }

  tcp::acceptor acceptor_;
  chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }

    boost::asio::io_context io_context;

    std::list<chat_server> servers;
    for (int i = 1; i < argc; ++i) {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_context, endpoint);
    }

    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  system("pause");
  return 0;
}