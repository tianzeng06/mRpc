#include "evrpc.pb.h"
#include "rpcserver.h"
#include <iostream>
#include <unistd.h>
#include <glog/logging.h>

using namespace mRpc;
using namespace std;

class TestMessageImpl : public TestMessage
{
 public:
	virtual void Put(::google::protobuf::RpcController* controller,const mRpc::PutRequest* request,mRpc::PutResult* response,::google::protobuf::Closure* done)
	{
		cout<<request->DebugString() <<endl;
		Rvalue *resVal = response->add_rvals();
      
		resVal->set_success(true);
		resVal->set_errmsg("no error!");
		done->Run();
		return;
	}
	virtual void Get(::google::protobuf::RpcController* controller,const mRpc::GetRequest* request,mRpc::GetResult* response,::google::protobuf::Closure* done)
	{
		cout<<request->DebugString() <<endl;
		Site *sit = response->add_sites();
		sit->set_key(request->keys(1));
		sit->set_value("value1");
		done->Run();
		return;
	}
};

int main()
{
	TestMessageImpl impl;
	RpcServer server(4, "127.0.0.1", 8009);
	server.registerService(&impl);
	server.start();
	google::protobuf::ShutdownProtobufLibrary();
	return;
}

