#include "rpcChannel.h"
#include "rpc.pb.h"
#include <google/protobuf/descriptor.h>

using namespace mRpc;
int RpcChannel::_id=0;

RpcChannel::RpcChannel(): rpcCodeDer(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1,std::placeholders::_2)),services(NULL)
{
	LOG(INFO) << "RpcChannel::Constructor Default " << this;
}

RpcChannel::RpcChannel(Conn* conn): rpcCodeDer(std::bind(&RpcChannel::onRpcMessage, this,std::placeholders::_1,std::placeholders::_2)),conn(conn),services(NULL)
{
	LOG(INFO) << "RpcChannel::Constructor with args" << this;
}

RpcChannel::~RpcChannel()
{
	LOG(INFO) << "RpcChannel::Destructor" << this;
	for (std::map<int64_t, OutstandingCall>::iterator it = outstandings.begin(); it != outstandings.end(); ++it)
	{
		OutstandingCall out = it->second;
		delete out.response;
		delete out.done;
	}
}

// Call the given method of the remote service.  The signature of this
// procedure looks the same as Service::CallMethod(), but the requirements
// are less strict in one important way:  the request and response objects
// need not be of any specific class as long as their descriptors are
// method->input_type() and method->output_type().
void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ::google::protobuf::Closure* done)
{
	RpcMessage message;//消息类型
	message.Clear();
	message.set_type(REQUEST);
	//int64_t id = id_.incrementAndGet();
	int64_t id = ++_id;
	message.set_id(id);
	message.set_service(method->service()->full_name());
	message.set_method(method->name());
	message.set_request(request->SerializeAsString()); // FIXME: error check

	OutstandingCall out = { response, done };
	{
		std::lock_guard<std::mutex> lk(mutex);
		outstandings[id] = out;
	}
	rpcCodeDer.send(conn, message);
}

void RpcChannel::onMessage(Conn* conn)
{
	if (conn)
		rpcCodeDer.onMessage(conn);
	else
		LOG(INFO) << "RpcChannel::onMessage Conn is null";
	return;
}

void RpcChannel::onRpcMessage(Conn* conn, const RpcMessagePtr& messagePtr)
{
	assert(conn == conn);
	//message.DebugString().c_str()
	
	RpcMessage& message = *messagePtr;
	if (message.type() == RESPONSE)
	{
		int64_t id = message.id();
		assert(message.has_response() || message.has_error());

		OutstandingCall out = { NULL, NULL };
		{
			std::lock_guard<std::mutex> lk(mutex);
			std::map<int64_t, OutstandingCall>::iterator it = outstandings.find(id);
			if (it != outstandings.end())
			{
				out = it->second;
				outstandings.erase(it);
			}
		}

		if (out.response)
		{
			std::unique_ptr<google::protobuf::Message> d(out.response);
			if (message.has_response())
				out.response->ParseFromString(message.response());

			if (out.done)
				out.done->Run();
		}
	}
	else if (message.type() == REQUEST)
	{
		// FIXME: extract to a function
		ErrorCode error = WRONG_PROTO;
		if (services)
		{
			std::map<std::string, google::protobuf::Service*>::const_iterator it = services->find(message.service());
			if (it != services->end())
			{
				google::protobuf::Service* service = it->second;
				assert(service);
				const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
				const google::protobuf::MethodDescriptor* method= desc->FindMethodByName(message.method());
				if (method)
				{
					std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
					if (request->ParseFromString(message.request()))
					{
						google::protobuf::Message* response = service->GetResponsePrototype(method).New();
						//response is deleted in doneCallback
						int64_t id = message.id();
						service->CallMethod(method, NULL, get_pointer(request), response,NewCallback(this, &RpcChannel::doneCallback, response, id));
						error = NO_ERROR;
					}
					else
						error = INVALID_REQUEST;
				}
				else
					error = NO_METHOD;
			}
			else
				error = NO_SERVICE;
		}
		else
			error = NO_SERVICE;

		if (error != NO_ERROR)
		{
			RpcMessage response;
			response.set_type(RESPONSE);
			response.set_id(message.id());
			response.set_error(error);
			rpcCodeDer.send(conn, response);
		}
	}
	else if (message.type() == ERROR)
	{
		LOG(INFO) << "message.type() is error";
		return;
	}
	return;
}

void RpcChannel::doneCallback(::google::protobuf::Message* response, int64_t id)
{
	assert(response&&id);

	std::unique_ptr<google::protobuf::Message> d(response);
	RpcMessage message;
	message.set_type(RESPONSE);
	message.set_id(id);
	message.set_response(response->SerializeAsString()); // FIXME: error check
	rpcCodeDer.send(conn, message);

	return;
}
