#ifndef RPC_RPCCHANNEL_H
#define RPC_RPCCHANNEL_H

#include "rpcCode.h"
#include "atomic.h"

#include <google/protobuf/service.h>

#include <map>
#include <mutex>
/*
namespace google
{
	namespace protobuf
	{
		// Defined in other files.
		class Descriptor;            // descriptor.h
		class ServiceDescriptor;     // descriptor.h
		class MethodDescriptor;      // descriptor.h
		class Message;               // message.h

		class Closure;

		class RpcController;
		class Service;
	}  // namespace protobuf
}  // namespace google

*/
namespace mRpc
{
// Abstract interface for an RPC channel.  An RpcChannel represents a
// communication line to a Service which can be used to call that Service's
// methods.  The Service may be running on another machine.  Normally, you
// should not call an RpcChannel directly, but instead construct a stub Service
// wrapping it.  Example:
// FIXME: update here
//   RpcChannel* channel = new MyRpcChannel("remotehost.example.com:1234");
//   MyService* service = new MyService::Stub(channel);
//   service->MyMethod(request, &response, callback);
class RpcChannel : public ::google::protobuf::RpcChannel
{
	public:
		RpcChannel();
		explicit RpcChannel(Conn* conn);
		~RpcChannel();

		void setConnection(Conn* conn)
		{
			conn = conn;
		}

		void setServices(const std::map<std::string, ::google::protobuf::Service*>* services)
		{
			services = services;
		}

		// Call the given method of the remote service.  The signature of this
		// procedure looks the same as Service::CallMethod(), but the requirements
		// are less strict in one important way:  the request and response objects
		// need not be of any specific class as long as their descriptors are
		// method->input_type() and method->output_type().
		void CallMethod(const ::google::protobuf::MethodDescriptor* method,
						::google::protobuf::RpcController* controller,
						const ::google::protobuf::Message* request,
						::google::protobuf::Message* response,
						::google::protobuf::Closure* done);
		void onMessage(Conn* conn);

	private:
		void onRpcMessage(Conn* conn, const RpcMessagePtr& messagePtr);
		void doneCallback(::google::protobuf::Message* response, int64_t id);
		struct OutstandingCall
		{
			::google::protobuf::Message* response;
			::google::protobuf::Closure* done;
		};

		RpcCodeDerived rpcCodeDer;
		Conn* conn;
		static int _id;
		std::mutex mutex;
		std::map<int64_t, OutstandingCall> outstandings;
		const std::map<std::string, ::google::protobuf::Service*>* services;
};
	typedef std::shared_ptr<RpcChannel> RpcChannelPtr;
	 //int RpcChannel::_id=0;
}
#endif  // RPC_RPCCHANNEL_H
