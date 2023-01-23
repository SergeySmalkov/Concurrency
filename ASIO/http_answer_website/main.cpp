#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <memory>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <limits>

class RequestData {
public:
    RequestData(std::string data) : data_(std::move(data)){
        parse();
    }

    void parse() {
        auto lines = split(data_, '\n');
        auto first_line = split(lines[0], ' ');
        type_ = first_line[0];
        path_ = first_line[1];
        http_version_ = first_line[2];
        for (int i = 1; i < lines.size(); ++i) {
            auto header = split(lines[i], ':');
            headers_[header[0]] = header[1].substr(1);
        }
    }

    std::string answer() {
      //path to www/
        std::string home = "/home/cheytakker/CLionProjects/asio_http_answer/www";
        std::ifstream fr(home + "/" + path_, std::ios_base::binary);
        fr.seekg(0, std::ios_base::end);
        size_t length = fr.tellg();
        fr.seekg(0, std::ios_base::beg);
        std::vector<char> buffer;
        buffer.reserve(length);
        std::copy(std::istreambuf_iterator<char>(fr),
                  std::istreambuf_iterator<char>(),
                  std::back_inserter(buffer));
        std::string answ = "HTTP/1.1 200 OK\n";
        if (path_.back() == 'g') {
            answ += "Content-Type:  image/jpeg\n";
        }
        // answ += "Content-Type: text/html;charset=utf-8\n";
        answ += "Content-Length: " + std::to_string(buffer.size()) + "\n";
        answ += "Connection: Keep-Alive\n";
        answ += "Server: Insta\n\n";;
        std::transform(buffer.begin(), buffer.end(), std::back_inserter(answ), [](char c) { return c; });
        return answ;
    }

    std::vector<std::string> split(const std::string& data, char ch) {
        std::string temp;
        std::vector<std::string> data_vec;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == ch) {
                data_vec.emplace_back(std::move(temp));
                temp.clear();
            } else {
                temp += data[i];
            }
        }
        if (!temp.empty()) {
            data_vec.emplace_back(std::move(temp));
        }
        return data_vec;
    }

    std::string& operator[](std::string key) {
        return headers_[key];
    }
private:
    std::unordered_map<std::string, std::string> headers_;
    std::string type_;
    std::string path_;
    std::string http_version_;
    std::string data_;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(asio::ip::tcp::socket socket) : socket_(std::move(socket)) {
    }

    void Start() {
        DoRead();
    }

private:
    void DoRead() {
        auto self(shared_from_this());
        socket_.async_read_some(
                asio::buffer(data_, max_length),
                [this, self] (std::error_code ec, std::size_t length) {
                    if (!ec) {
                        std::string last;
                        for (int i = 0; i < length; i++) {
                            request_ += data_[i];
                            int ind = std::max(0, (int)request_.length() - 4);
                            last = request_.substr(ind);
                            if (last == "\r\n\r\n") {
                                break;
                            }
                        }
                        if (last == "\r\n\r\n") {
                            RequestData h(request_.substr(0, request_.length() - 4));
                            response_ = h.answer();
                            std::cout << response_ << std::endl;
                            DoWrite();
                        } else {
                            DoRead();
                        }
                    }
                }
        );
    }

    void DoWrite() {
        auto self(shared_from_this());
        asio::async_write(
                socket_,
                asio::buffer(response_, response_.length()),
                [this, self] (std::error_code ec, std::size_t length) {
                    if (!ec) {
                        //socket_.close();
                    }
                }
        );
    }

    asio::ip::tcp::socket socket_;

    static constexpr size_t max_length = 1024;
    std::string request_;
    std::string response_;
    char data_[max_length];
};



class Server {
public:
    Server(asio::io_context& io_context, short port)
            : acceptor_(
            io_context,
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)
    ),
              socket_(io_context){
        do_accept();
    }

    void do_accept() {
        acceptor_.async_accept(socket_, [this](std::error_code ec) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket_))->Start();
            }
            do_accept();
        });
    }

private:
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
};

int main() {
    try {
        asio::io_context io_context;
        Server s(io_context, 8081);
        io_context.run();
    } catch (const std::exception& e) {

        std::cout << e.what() << std::endl;
    }
}
