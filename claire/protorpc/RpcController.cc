// Copyright (c) 2013 The claire-protorpc Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/protorpc/RpcController.h>
#include <claire/protorpc/RpcUtil.h>
#include <claire/protorpc/rpcmessage.pb.h> // protoc-rpc-gen

namespace claire {

RpcController::RpcController()
    : error_(RPC_SUCCESS) {}

void RpcController::Reset()
{
    error_ = RPC_SUCCESS;
    reason_.clear();

    context_ = boost::any();
}

bool RpcController::Failed() const
{
    return error_ != RPC_SUCCESS;
}

void RpcController::SetFailed(const std::string& reason)
{
    SetFailed(RPC_ERROR_INTERNAL_ERROR, reason);
}

std::string RpcController::ErrorText() const
{
    std::string output;
    if (Failed())
    {
        output = ErrorCodeToString(error_);

        if (!reason_.empty())
        {
            output += ": " + reason_;
        }
    }
    return output;
}

} // namespace claire
