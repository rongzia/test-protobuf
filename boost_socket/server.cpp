//
// Created by rrzhang on 2020/10/6.
//

#include <iostream>
#include "boost/asio.hpp"

int main() {
    boost::asio::io_service io;
    boost::asio::ip::tcp::acceptor acceptor(io,
        boost::asio::ip::tcp::endpoint( boost::asio::ip::address::from_string("0.0.0.0"), 30000));


    boost::asio::ip::tcp::socket sock(io);
    acceptor.accept(sock);


    while (true) {
        /// 长度
        char data_len[sizeof(size_t)];
        sock.receive(boost::asio::buffer(data_len));
        std::cout << "data len: " << *(size_t*)data_len << std::endl;
        /// 实际数据
        std::vector<char> data(*(size_t*)data_len, 0);
        size_t recive_size = sock.receive(boost::asio::buffer(data));
        std::cout << "recive size: " << recive_size << ", recive data: " << data.data() << std::endl;

        std::string return_data = std::string(reinterpret_cast<const char*>(&data_len), sizeof(size_t)) + data.data();
        size_t return_size = sock.send(boost::asio::buffer(return_data));
        std::cout << "return size: " << return_size << ", return data: " << return_data << std::endl;
    }

    return 0;
}