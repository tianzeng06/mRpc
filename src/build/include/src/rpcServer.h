#ifndef RPC_RPCSERVER_H
#define RPC_RPCSERVER_H

#include "tcpServer.h"
#include <map>
#include <google/protobuf/service.h>
namespace mRpc
{
	class RpcServer
	{
		public:
			RpcServer(int threads, const std::string& ip, int port);

			void registerService(::google::protobuf::Service*);
			void start();
		private:
			void onConnection(Conn* conn);
			TcpServer server;
			std::map<std::string, ::google::protobuf::Service*> services;
	};
}
#endif  // RPC_RPCSERVER_H
