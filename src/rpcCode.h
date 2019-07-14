#ifndef RPC_RPCCODEC_H
#define RPC_RPCCODEC_H

#include "codeClite.h"

namespace mRpc
{
	class Buffer;
	class TcpConnection;
	typedef std::shared_ptr<Conn *> TcpConnectionPtr;

	class RpcMessage;
	typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
	extern const char rpctag[];

	typedef ProtobufCodecLiteDerived<RpcMessage, rpctag> RpcCodeDerived;

}

#endif  // MUDUO_NET_PROTORPC_RPCCODEC_H
