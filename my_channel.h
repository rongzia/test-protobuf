#include "google/protobuf/service.h"
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

class MyChannel : public ::google::protobuf::RpcChannel {
public:
    //init传入ip:port，网络交互使用boost.asio
    void init(const std::string& ip, const int port) {
        _io = boost::make_shared<boost::asio::io_service>();
        _sock = boost::make_shared<boost::asio::ip::tcp::socket>(*_io);
        boost::asio::ip::tcp::endpoint ep(
                boost::asio::ip::address::from_string(ip), port);
        _sock->connect(ep);
    }
 
    //EchoService_Stub::Echo会调用Channel::CallMethod
    //其中第一个参数MethodDescriptor* method，可以获取service-name method-name
    virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
            ::google::protobuf::RpcController* /* controller */,
            const ::google::protobuf::Message* request,
            ::google::protobuf::Message* response,
            ::google::protobuf::Closure*) {
        //request数据序列化
        std::string serialzied_data = request->SerializeAsString();
 
        //获取service-name method-name，填充到rpc_meta
        myrpc::RpcMeta rpc_meta;
        rpc_meta.set_service_name(method->service()->name());
        rpc_meta.set_method_name(method->name());
        rpc_meta.set_data_size(serialzied_data.size());
 
        //rpc_meta序列化
        std::string serialzied_str = rpc_meta.SerializeAsString();
 
        //获取rpc_meta序列化数据大小，填充到数据头部，占用4个字节
        int serialzied_size = serialzied_str.size();
        serialzied_str.insert(0, std::string((const char*)&serialzied_size, sizeof(int)));
        //尾部追加request序列化后的数据
        serialzied_str += serialzied_data;
 
        //发送全部数据:
        //|rpc_meta大小（定长4字节)|rpc_meta序列化数据（不定长）|request序列化数据（不定长）|
        _sock->send(boost::asio::buffer(serialzied_str));
 
        //接收4个字节：序列化的resp数据大小
        char resp_data_size[sizeof(int)];
        _sock->receive(boost::asio::buffer(resp_data_size));
 
        //接收N个字节：N=序列化的resp数据大小
        int resp_data_len = *(int*)resp_data_size;
        std::vector<char> resp_data(resp_data_len, 0);
        _sock->receive(boost::asio::buffer(resp_data));
 
        //反序列化到resp
        response->ParseFromString(std::string(&resp_data[0], resp_data.size()));
    }
 
private:
    boost::shared_ptr<boost::asio::io_service> _io;
    boost::shared_ptr<boost::asio::ip::tcp::socket> _sock;
};