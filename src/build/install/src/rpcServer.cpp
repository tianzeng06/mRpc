#include "rpcServer.h"
#include "tcpServer.h"
#include "rpcChannel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

using namespace mRpc;

RpcServer::RpcServer(int threads,const std::string& ip, int port):server(threads, ip, port)
{
	//һ���µ����ӵ���,ִ�лص�����
	server.setConnectionCallback(std::bind(&RpcServer::onConnection, this, std::placeholders::_1));
	LOG(INFO) << "RpcServer contruction success";
}
void RpcServer::onConnection(Conn* conn)
{
	if (conn)
	{
		LOG(INFO) << "Socketfd:" << conn->getFd() << "\tthreadid:" << conn->getThread()->thread_id << std::endl;

		RpcChannelPtr channel(new RpcChannel(conn));
		channel->setServices(&services);
		server.setReadCallback(std::bind(&RpcChannel::onMessage, get_pointer(channel), std::placeholders::_1));//��channel�ж�����,RpcChannel::onMessage
	   //conn->setContext(channel); 
	}
	else
		LOG(INFO) << "RpceServer::onConnection conn is null";

	return;
}

void RpcServer::registerService(google::protobuf::Service* service)
{
	if (service)
	{
		const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
		services[desc->full_name()] = service;
	}
	else
		LOG(INFO) << "registerService is null";
	return;
}

void RpcServer::start()
{
	server.startRun();
	return;
}
