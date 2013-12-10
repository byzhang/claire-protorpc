// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors

#include <claire/protorpc/RpcServer.h>

#include <ctemplate/template.h>

#include <map>
#include <string>

#include <boost/bind.hpp>

#include <claire/common/logging/Logging.h>
#include <claire/common/metrics/Counter.h>
#include <claire/common/metrics/Histogram.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/protobuf/ProtobufIO.h>

#include <claire/netty/InetAddress.h>
#include <claire/netty/http/HttpServer.h>
#include <claire/netty/http/HttpRequest.h>
#include <claire/netty/http/HttpResponse.h>
#include <claire/netty/http/HttpConnection.h>

#include <claire/netty/inspect/FlagsInspector.h>
#include <claire/netty/inspect/PProfInspector.h>
#include <claire/netty/inspect/StatisticsInspector.h>

#include <claire/protorpc/RpcCodec.h>
#include <claire/protorpc/RpcAssets.h>     // gen-assets
#include <claire/protorpc/rpcmessage.pb.h> // protoc-rpc-gen
#include <claire/protorpc/BuiltinService.h>

namespace claire {

static const char* kRpcServicePath = "/__protorpc__";

class RpcServer::Impl : boost::noncopyable
{
public:
    Impl(EventLoop* loop, const InetAddress& listen_address)
        : loop_(loop),
          server_(loop, listen_address, "RpcServer"),
          flags_(&server_),
          pprof_(&server_),
          statistics_(&server_),
          total_request_("protorpc.RpcServer.total_request"),
          total_response_("protorpc.RpcServer.total_response"),
          failed_request_("protorpc.RpcServer.failed_request")
    {
        codec_.set_message_callback(
            boost::bind(&Impl::OnRequest, this, _1, _2));

        server_.set_headers_callback(
            boost::bind(&Impl::OnHeaders, this, _1));

        server_.Register("/form",
                         boost::bind(&Impl::OnForm, this, _1),
                         false);
        server_.RegisterAsset("/static/forms.js",
                              ASSET_forms_js,
                              ASSET_forms_js_length);

        server_.Register("/protorpc",
                         boost::bind(&Impl::OnJson, this, _1),
                         true);

        RegisterService(&builtin_service_);
    }

    void Start()
    {
        for (auto it = services_.begin(); it != services_.end(); ++it)
        {
            LOG(DEBUG) << "service " << it->first << " registered.";
            //FIXME: dump method
        }

        builtin_service_.set_services(services_);
        server_.Start();
    }

    void set_num_threads(int num_threads)
    {
        server_.set_num_threads(num_threads);
    }

    void RegisterService(Service* service)
    {
        services_[service->GetDescriptor()->full_name()] = service;
    }

    void OnHeaders(const HttpConnectionPtr& connection)
    {
        auto request = connection->mutable_request();

        //FIXME: verify user first
        if (request->method() == HttpRequest::kPost &&
            request->uri().path() == kRpcServicePath)
        {
            connection->set_body_callback(
                boost::bind(&Impl::OnMessage, this, _1, _2));

            Buffer buffer;
            buffer.Append("HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\n\r\n");
            connection->Send(&buffer);
        }
    }

    void OnMessage(const HttpConnectionPtr& connection, Buffer* buffer)
    {
        codec_.ParseFromBuffer(connection, buffer);
    }

    void OnRequest(const HttpConnectionPtr& connection, const RpcMessage& message)
    {
        total_request_.Increment();

        RpcControllerPtr controller(new RpcController());
        FillContext(controller, message, connection);
        if (!message.has_request())
        {
            controller->SetFailed(RPC_ERROR_INVALID_REQUEST);
            OnRequestComplete(controller, NULL);
            return ;
        }

        auto it = services_.find(message.service());
        if (it != services_.end())
        {
            auto service = it->second;
            DCHECK(service != NULL);

            const auto method = service->GetDescriptor()->FindMethodByName(message.method());
            if (method)
            {
                ::google::protobuf::MessagePtr request(service->GetRequestPrototype(method).New());
                if (!request->ParseFromString(message.request()))
                {
                    controller->SetFailed(RPC_ERROR_PARSE_FAIL);
                    OnRequestComplete(controller, NULL);
                }
                else
                {
                    service->CallMethod(method,
                                        controller,
                                        request,
                                        &service->GetResponsePrototype(method),
                                        boost::bind(&Impl::OnRequestComplete, this, _1, _2));
                }
            }
            else
            {
                controller->SetFailed(RPC_ERROR_INVALID_METHOD);
                OnRequestComplete(controller, NULL);
            }
        }
        else
        {
            controller->SetFailed(RPC_ERROR_INVALID_SERVICE);
            OnRequestComplete(controller, NULL);
        }
    }

    void OnRequestComplete(RpcControllerPtr& controller,
                           const ::google::protobuf::Message* response)
    {
        auto context = boost::any_cast<const Context&>(controller->context());

        RpcMessage message;
        message.set_type(RESPONSE);
        message.set_id(context.id);

        if (controller->Failed())
        {
            message.set_error(static_cast<ErrorCode>(controller->ErrorCode())); // FIXME
            message.set_reason(controller->ErrorText());
        }
        else
        {
            message.set_response(response->SerializeAsString());
            if (context.compress_type != Compress_None)
            {
                message.set_compress_type(context.compress_type);
            }
        }

        Buffer buffer;
        codec_.SerializeToBuffer(message, &buffer);
        server_.SendByHttpConnectionId(context.connection_id, &buffer);

        total_response_.Increment();
        if (controller->Failed())
        {
            failed_request_.Increment();
        }
    }

    void OnForm(const HttpConnectionPtr& connection)
    {
        auto request = connection->mutable_request();
        if (request->method() != HttpRequest::kGet)
        {
            connection->OnError(HttpResponse::k405MethodNotAllowed,
                                "Only accept Get method");
            return ;
        }

        auto service_name = request->uri().get_parameter("service");
        auto method_name = request->uri().get_parameter("method");
        if ((service_name.empty() && !method_name.empty())
            && (!service_name.empty() && method_name.empty()))
        {
            connection->OnError(HttpResponse::k400BadRequest,
                                "Invalid service/method");
            return ;
        }

        ctemplate::TemplateDictionary dictionary("data");
        dictionary.SetValue("REGISTRY_NAME",
                            builtin_service_.GetDescriptor()->full_name());

        std::string output;
        if (service_name.empty() && method_name.empty())
        {
            dictionary.SetValue("HOST_NAME", request->get_header("HOST"));
            ctemplate::StringToTemplateCache("forms.html",
                                             ASSET_forms_html,
                                             ASSET_forms_html_length,
                                             ctemplate::STRIP_WHITESPACE);
            ctemplate::ExpandTemplate("forms.html",
                                      ctemplate::STRIP_WHITESPACE,
                                      &dictionary,
                                      &output);
        }
        else
        {
            dictionary.SetValue("SERVICE_NAME", service_name);
            dictionary.SetValue("METHOD_NAME", method_name);
            ctemplate::StringToTemplateCache("methods.html",
                                             ASSET_methods_html,
                                             ASSET_methods_html_length,
                                             ctemplate::STRIP_WHITESPACE);
            ctemplate::ExpandTemplate("methods.html",
                                      ctemplate::STRIP_WHITESPACE,
                                      &dictionary,
                                      &output);
        }

        HttpResponse response;
        response.mutable_body()->swap(output);
        connection->Send(&response);
    }

    void OnJson(const HttpConnectionPtr& connection)
    {
        if (connection->mutable_request()->method() != HttpRequest::kPost)
        {
            connection->OnError(HttpResponse::k405MethodNotAllowed,
                                "Only accept Post method");
            return ;
        }

        auto service_name = connection->mutable_request()->uri().get_parameter("service");
        auto method_name = connection->mutable_request()->uri().get_parameter("method");
        if (services_.find(service_name) == services_.end())
        {
            connection->OnError(HttpResponse::k400BadRequest,
                                "Invalid service name");
            return ;
        }

        auto service = services_[service_name];
        auto method = service->GetDescriptor()->FindMethodByName(method_name);
        if (!method)
        {
            connection->OnError(HttpResponse::k400BadRequest,
                                "Invalid method name");
            return ;
        }

        ::google::protobuf::MessagePtr request(service->GetRequestPrototype(method).New());
        if (!ParseFromJson(*(connection->mutable_request()->mutable_body()), get_pointer(request)))
        {
            connection->OnError(HttpResponse::k400BadRequest,
                                "Convert json data to protobuf failed");
            return ;
        }

        RpcControllerPtr controller(new RpcController()); // FIXME: controller pool?
        controller->set_context(connection->id());

        service->CallMethod(method,
                            controller,
                            request,
                            &service->GetResponsePrototype(method),
                            boost::bind(&Impl::OnJsonComplete, this, _1, _2));
        return ;
    }

    void OnJsonComplete(RpcControllerPtr& controller,
                        const ::google::protobuf::Message* message)
    {
        total_request_.Increment();

        auto connection_id = boost::any_cast<HttpConnection::Id>(controller->context());
        if (controller->Failed())
        {
            failed_request_.Increment();
            server_.OnError(connection_id,
                            HttpResponse::k500InternalServerError,
                            controller->ErrorText());
            return ;
        }

        std::string output;
        if (SerializeToJson(*message, &output))
        {
            HttpResponse response;
            response.AddHeader("Content-Type", "application/json");
            response.mutable_body()->swap(output);
            server_.SendByHttpConnectionId(connection_id, &response);
        }
        else
        {
            server_.OnError(connection_id,
                            HttpResponse::k500InternalServerError,
                            "SerializeToJson failed");
            failed_request_.Increment();
        }
    }

    void FillContext(RpcControllerPtr& controller,
                     const RpcMessage& message,
                     const HttpConnectionPtr& connection)
    {
        Context context;
        context.id = message.id();
        if (message.has_compress_type())
        {
            context.compress_type = message.compress_type();
        }

        context.connection_id = connection->id();
        controller->set_context(context);
    }

    struct Context
    {
        Context()
            : id(-1),
              compress_type(Compress_None),
              connection_id(-1)
        {}

        int64_t id;
        CompressType compress_type;
        HttpConnection::Id connection_id;
    };

    EventLoop* loop_;
    HttpServer server_;
    RpcCodec codec_;

    BuiltinServiceImpl builtin_service_;
    std::map<std::string, Service*> services_;

    FlagsInspector flags_;
    PProfInspector pprof_;
    StatisticsInspector statistics_;

    Counter total_request_;
    Counter total_response_;
    Counter failed_request_;
};

RpcServer::RpcServer(EventLoop* loop, const InetAddress& listen_address)
    : impl_(new Impl(loop, listen_address)) {}

RpcServer::~RpcServer() {}

void RpcServer::Start()
{
    impl_->Start();
}

void RpcServer::set_num_threads(int num_threads)
{
    impl_->set_num_threads(num_threads);
}

void RpcServer::RegisterService(Service* service)
{
    impl_->RegisterService(service);
}

} // namespace claire
