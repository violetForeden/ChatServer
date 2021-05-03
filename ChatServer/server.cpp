/*
        һ���򵥵����������
*/
#include "chat_message.hpp"
#include "SerilizationObject.hpp"

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
// ���࣬������
class chat_participant {
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message &msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

//----------------------------------------------------------------------

class chat_room {
public:
  // ����������
  void join(chat_participant_ptr participant) {
    participants_.insert(participant);
    //std::cout << "���������ң�\n";
    for (const auto &msg : recent_msgs_)
      participant->deliver(msg);
  }

  // �뿪������
  void leave(chat_participant_ptr participant) {
    participants_.erase(participant);
  }

  // �ַ���Ϣ
  void deliver(const chat_message &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto &participant :
         participants_) // ����ָ�뿽�����Ĵ������ָͨ���20������
      participant->deliver(msg);
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 }; // �������100����Ϣ
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------
// �ͻ��˻Ự��
class chat_session : public chat_participant,
                     public std::enable_shared_from_this<chat_session> {
public:
  chat_session(tcp::socket socket, chat_room &room)
      : socket_(std::move(socket)), room_(room) {}

  void start() {
    room_.join(shared_from_this()); // �������Ҽ���һ���ͻ���
    do_read_header();
  }

  void deliver(const chat_message &msg) {
    // ��һ����false
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
      // ��һ��
      do_write();
    }
  }

private:
  void do_read_header() {
    auto self(shared_from_this()); // ��ָֹ�뱻����
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && read_msg_.decode_header()) {
            //std::cout << "��ȡ��ͷ�ɹ���\n";
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
    //std::cout << "����do read body��" << std::endl;
    boost::asio::async_read(
        socket_, 
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          //std::cout << "���봦������ص���" << std::endl;
          if (!ec) {
            //room_.deliver(read_msg_);
            //std::cout << "��ȡ����ɹ���" << std::endl;
            handleMessage();
            do_read_header();
          } else {
            std::cout << "error: " << ec << std::endl;
            room_.leave(shared_from_this());
          }
        });
  }
  // ģ�����
  template <typename T> T toObject() {
    T obj;
    std::stringstream ss(std::string(read_msg_.body(),
                         read_msg_.body() + read_msg_.body_length()));
    boost::archive::text_iarchive oa(ss);
    oa &obj;
    return obj;
  }

  // ������Ϣ
  void handleMessage() { 
    if (read_msg_.type() == MT_BIND_NAME) {
      /*SBindName bindName;
      std::stringstream ss(read_msg_.body());
      boost::archive::text_iarchive oa(ss);
      oa &bindName;*/
      
      auto bindName = toObject<SBindName>();
      m_name = bindName.bindName();
    } else if (read_msg_.type() == MT_CHAT_INFO) {
      /*SChatInfo chat;
      std::stringstream ss(std::string(
          read_msg_.body(), read_msg_.body() + read_msg_.body_length()));
      boost::archive::text_iarchive oa(ss);
      oa &chat;*/
      auto chat = toObject<SChatInfo>();
      m_chatInformation = chat.chatInformation();

      auto rinfo = buildRoomInfo();
      chat_message msg;
      msg.setMessage(MT_ROOM_INFO, rinfo);
      room_.deliver(msg);
      
    } else {
      // not valid msg do nothing
    }
  }

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
    SRoomInfo roomInfo(m_name, m_chatInformation);
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa &roomInfo;
    return ss.str();
  }
};

//----------------------------------------------------------------------
// ��������
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
        std::cout << "�ͻ��˽�������" << std::endl;
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