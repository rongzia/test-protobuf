#include <iostream>
#include "rpc_meta.pb.h"
#include "google/protobuf/service.h"
#include "google/protobuf/stubs/common.h"
#include "boost/asio.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"

namespace myrpc {

    class DonePara{
    public:
        DonePara(
                ::google::protobuf::Message *recv_msg,
                ::google::protobuf::Message *resp_msg,
                const boost::shared_ptr<boost::asio::ip::tcp::socket> sock)
                : resp_msg_(resp_msg), recv_msg_(recv_msg), sock_(sock){}
        ::google::protobuf::Message *recv_msg_;
        ::google::protobuf::Message *resp_msg_;
        const boost::shared_ptr<boost::asio::ip::tcp::socket> sock_;
    };

    class Controller : public ::google::protobuf::RpcController {

    public:
        virtual void Reset() {};

        virtual bool Failed() const { return false; };

        virtual std::string ErrorText() const { return ""; };

        virtual void StartCancel() {};

        virtual void SetFailed(const std::string & /* reason */) {};

        virtual bool IsCanceled() const { return false; };

        virtual void NotifyOnCancel(::google::protobuf::Closure * /* callback */) {};
    };//Controller

    class Channel : public ::google::protobuf::RpcChannel {
    public:
        void init(const std::string &ip, const int port) {
            _io = boost::make_shared<boost::asio::io_service>();
            _sock = boost::make_shared<boost::asio::ip::tcp::socket>(*_io);
            boost::asio::ip::tcp::endpoint ep(
                    boost::asio::ip::address::from_string(ip), port);
            _sock->connect(ep);
        }

        virtual void CallMethod(const ::google::protobuf::MethodDescriptor *method,
                                ::google::protobuf::RpcController * /* controller */,
                                const ::google::protobuf::Message *request,
                                ::google::protobuf::Message *response,
                                ::google::protobuf::Closure *) {
            std::string serialzied_data = request->SerializeAsString();

            myrpc::RpcMeta rpc_meta;
            rpc_meta.set_service_name(method->service()->name());
            rpc_meta.set_method_name(method->name());
            rpc_meta.set_data_size(serialzied_data.size());

            std::cout << "CallMethod, service_name:" << method->service()->name() << std::endl;
            std::cout << "CallMethod,  method_name:" << method->name() << std::endl;

            std::string serialzied_str = rpc_meta.SerializeAsString();

            size_t serialzied_size = serialzied_str.size();
            serialzied_str.insert(0, std::string((const char *) &serialzied_size, sizeof(size_t)));
            serialzied_str += serialzied_data;

            _sock->send(boost::asio::buffer(serialzied_str));

            char resp_data_size[sizeof(size_t)];
            _sock->receive(boost::asio::buffer(resp_data_size));

            size_t resp_data_len = *(size_t *) resp_data_size;
            std::vector<char> resp_data(resp_data_len, 0);
            _sock->receive(boost::asio::buffer(resp_data));

            response->ParseFromString(std::string(&resp_data[0], resp_data.size()));
        }

    private:
        boost::shared_ptr<boost::asio::io_service> _io;
        boost::shared_ptr<boost::asio::ip::tcp::socket> _sock;
    };//mychannel

    class Server {
    public:
        void add(::google::protobuf::Service *service) {
            ServiceInfo service_info;
            service_info.service = service;
            service_info.sd = service->GetDescriptor();
            for (int i = 0; i < service_info.sd->method_count(); ++i) {
                service_info.mds[service_info.sd->method(i)->name()] = service_info.sd->method(i);
            }

            _services[service_info.sd->name()] = service_info;
        }

        void start(const std::string &ip, const int port);

    private:
        void dispatch_msg(
                const std::string &service_name,
                const std::string &method_name,
                const std::string &serialzied_data,
                const boost::shared_ptr<boost::asio::ip::tcp::socket> &sock);

        void on_resp_msg_filled(DonePara para);

        void pack_message(
                const ::google::protobuf::Message *msg,
                std::string *serialized_data) {
            size_t serialized_size = msg->ByteSizeLong();
            serialized_data->assign(
                    (const char *) &serialized_size,
                    sizeof(serialized_size));
            msg->AppendToString(serialized_data);
        }

    private:
        struct ServiceInfo {
            ::google::protobuf::Service *service;
            const ::google::protobuf::ServiceDescriptor *sd;
            std::map<std::string, const ::google::protobuf::MethodDescriptor *> mds;
        };//ServiceInfo

        //service_name -> {Service*, ServiceDescriptor*, MethodDescriptor* []}
        std::map<std::string, ServiceInfo> _services;
    };//Server

    void Server::start(const std::string &ip, const int port) {
        boost::asio::io_service io;
        boost::asio::ip::tcp::acceptor acceptor(
                io,
                boost::asio::ip::tcp::endpoint(
                        boost::asio::ip::address::from_string(ip),
                        port));

        while (true) {
            auto sock = boost::make_shared<boost::asio::ip::tcp::socket>(io);
            acceptor.accept(*sock);

            std::cout << "recv from client:"
                      << sock->remote_endpoint().address()
                      << std::endl;

            //接收meta长度
            char meta_size[sizeof(size_t)];
            sock->receive(boost::asio::buffer(meta_size));

            size_t meta_len = *(size_t *) (meta_size);

            //接收meta数据
            std::vector<char> meta_data(meta_len, 0);
            sock->receive(boost::asio::buffer(meta_data));

            myrpc::RpcMeta meta;
            meta.ParseFromString(std::string(&meta_data[0], meta_data.size()));

            //接收数据本身
            std::vector<char> data(meta.data_size(), 0);
            sock->receive(boost::asio::buffer(data));

            //
            dispatch_msg(
                    meta.service_name(),
                    meta.method_name(),
                    std::string(&data[0], data.size()),
                    sock);
        }
    }


    void Server::dispatch_msg(
            const std::string &service_name,
            const std::string &method_name,
            const std::string &serialzied_data,
            const boost::shared_ptr<boost::asio::ip::tcp::socket> &sock) {
        auto service = _services[service_name].service;
        auto md = _services[service_name].mds[method_name];

        std::cout << "recv service_name:" << service_name << std::endl;
        std::cout << "recv method_name:" << method_name << std::endl;
        std::cout << "recv type:" << md->input_type()->name() << std::endl;
        std::cout << "resp type:" << md->output_type()->name() << std::endl;

        auto recv_msg = service->GetRequestPrototype(md).New();
        recv_msg->ParseFromString(serialzied_data);
        auto resp_msg = service->GetResponsePrototype(md).New();

        Controller controller;
        DonePara * para = new DonePara(recv_msg, resp_msg, sock);
        auto done = ::google::protobuf::NewCallback(
                this,
                &Server::on_resp_msg_filled,
                *para);
        service->CallMethod(md, &controller, recv_msg, resp_msg, done);
    }

    void Server::on_resp_msg_filled(DonePara para) {
        boost::scoped_ptr<::google::protobuf::Message> recv_msg_guard(para.recv_msg_);
        boost::scoped_ptr<::google::protobuf::Message> resp_msg_guard(para.resp_msg_);

        std::string resp_str;
        pack_message(para.resp_msg_, &resp_str);

        para.sock_->send(boost::asio::buffer(resp_str));
    }
/* vim: set ts=4 sw=4 sts=4 tw=100 */
}