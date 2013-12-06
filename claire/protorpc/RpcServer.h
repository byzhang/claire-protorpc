// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors

#ifndef _CLAIRE_PROTORPC_RPCSERVER_H_
#define _CLAIRE_PROTORPC_RPCSERVER_H_

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

class Service;
class EventLoop;
class InetAddress;

class RpcServer : boost::noncopyable
{
public:
    RpcServer(EventLoop* loop, const InetAddress& listen_address);
    ~RpcServer();

    void Start();
    void set_num_threads(int num_threads);
    void RegisterService(Service* service);

private:
    class Impl;
    boost::shared_ptr<Impl> impl_;
};

} // namespace claire

#endif // _CLAIRE_PROTORPC_RPCSERVER_H_
