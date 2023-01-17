#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(asio::ip::tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }
private:
    void do_read() {
        std::shared_ptr<Session> self(shared_from_this());

        socket_.async_read_some(asio::buffer(data_, max_length),[this, self] (std::error_code ec, std::size_t length){
            if (!ec) {
                std::cout << data_;
                do_write(length);
            }
        });
    }

    void do_write(std::size_t length) {
        std::shared_ptr<Session> self(shared_from_this());

        asio::async_write(socket_, asio::buffer(data_, length),[this, self] (std::error_code ec, std::size_t /*length*/){
          if (!ec) {
              std::cout << data_;
              do_read();
          }
        });
    }
    asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length]{};
};

class Server {
public:
    Server(asio::io_context& io_context, short port) :
    acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    socket_(io_context) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(socket_, [this](std::error_code ec) {
           if (!ec) {
               std::make_shared<Session>(std::move(socket_))->start();
           }
           do_accept();
        });
    }

    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
};

int main() {
    asio::io_context io_context;
    Server server(io_context, 8080);
    io_context.run();
  
    // use telnet 127.0.0.1 8080 from commanf line or the browser 127.0.0.1:8080 to connect
  
    return 0;
}
