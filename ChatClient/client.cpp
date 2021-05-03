/*
        一个简单的聊天客户端
*/

#include "chat_message.hpp"
//#include "SerilizationObject.hpp"
#include "JsonObject.h"
#include "Protocal.pb.h"

#include <boost/asio.hpp>

#include <deque>
#include <iostream>
#include <thread>

#include <cstdlib>

#pragma comment(lib, "libprotobufd.lib")

using boost::asio::ip::tcp;

using chat_message_queue = std::deque<chat_message>;

class chat_client {
public:
  chat_client(boost::asio::io_context &io_context,
              const tcp::resolver::results_type &endpoints)
      : io_context_(io_context), socket_(io_context) {
    do_connect(endpoints);
  }

  void write(const chat_message &msg) {
    boost::asio::post(io_context_, [this, msg]() {
      bool write_in_progress = !write_msgs_.empty();
      write_msgs_.push_back(msg);
      if (!write_in_progress) {
        // 如果程序不在写的过程中，执行do_write()
        do_write();
      }
    }); // post：将事件注册进io_context，由io_context调度（事件在io_context.run()所在线程执行，保证线程安全）
  }

  void close() {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

private:
  void do_connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(
        socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint) {
          if (!ec) {
            do_read_header();
          }
        });
  }

  void do_read_header() {
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && read_msg_.decode_header()) {
            //std::cout << "读取包头成功！\n";
            do_read_body();
          } else {
            socket_.close(); // 出错，关闭socket连接
          }
        });
  }

  void do_read_body() {
    boost::asio::async_read(
        socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            //std::cout << "读取包体成功！\n";
            if (read_msg_.type() == MT_ROOM_INFO) {
              //std::stringstream ss( // 将read_msh_构造string流
              //    std::string(read_msg_.body(),
              //                read_msg_.body() + read_msg_.body_length()));
              //ptree tree;
              //boost::property_tree::read_json(ss, tree);
              std::string buffer(read_msg_.body(),
                                 read_msg_.body() + read_msg_.body_length());
              PRoomInformation roomInfo;
              auto ok = roomInfo.ParseFromString(buffer);   // protobuf 从字符串解析数据
              if (ok) {
                std::cout << "client: '";
                std::cout << roomInfo.name();
                std::cout << "' says '";
                std::cout << roomInfo.information();
                std::cout << "'\n";
              }
              //boost::archive::text_iarchive oa(ss); //  使用text_iarchive 将ss流进行序列化赋值
              //oa &info;
              //std::cout << "client: '";
              //std::cout << info.name();
              //std::cout << "' says '";
              //std::cout << info.information();
              //std::cout << "'\n";
            }
            do_read_header();
            /*std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
            do_read_header();*/
          } else {
            std::cout << "error: " << ec << std::endl;
            socket_.close();
          }
        });
  }

  void do_write() {
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_msgs_.front().data(),
                            write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
              //std::cout << "消息发送成功！\n";
              do_write();
            }
          } else {
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_context &io_context_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

int main(int argc, char *argv[]) {
  try {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    if (argc != 3) {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    chat_client c(io_context, endpoints);

    std::thread t([&io_context]() { io_context.run(); }); // 另开线程跑io

    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1)) {
      chat_message msg;
      auto type = 0;
      std::string input(line, line + std::strlen(line));
      std::string output;
      if (parseMessage3(input, &type, output)) {
        msg.setMessage(type, output.data(), output.size());
        c.write(msg);
        std::cout << "write message for server " << output.size()
                  << std::endl;
      }
      /*msg.body_length(std::strlen(line));
      std::memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      c.write(msg);*/
    }

    c.close(); // 释放相关资源，让io_context.run()退出
    t.join();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}