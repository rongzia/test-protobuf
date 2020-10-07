//
// Created by rrzhang on 2020/10/6.
//
#include <iostream>
#include "boost/asio.hpp"

int main() {

    boost::asio::io_service *io = new boost::asio::io_service();
    boost::asio::ip::tcp::socket *sock = new boost::asio::ip::tcp::socket(*io);

    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30000);
    sock->connect(ep);

    for(size_t i = 0; i < 10; i++) {
        std::string data = "hello" + std::to_string(i);
        std::cout << "data: " << data << ", size: " << data.size() << std::endl;
        size_t data_len = data.size();
        data.insert(0, std::string(reinterpret_cast<const char*>(&data_len), sizeof(size_t)));

        size_t send_size = sock->send(boost::asio::buffer(data));
        std::cout << "send data: " << data << ", send size: " << send_size << std::endl;

        /// 长度
        char data_length[sizeof(size_t)];
        sock->receive(boost::asio::buffer(data_length));
        std::cout << "data len: " << *(size_t*)data_length << std::endl;
        /// 实际数据
        std::vector<char> back_data(*(size_t*)data_length, 0);
        size_t recive_size = sock->receive(boost::asio::buffer(back_data));
        std::cout << "return size: " << recive_size << ", return data: " << data.data() << std::endl;

    }

    return 0;
}