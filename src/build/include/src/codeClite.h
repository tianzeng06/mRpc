#ifndef RPC_CODEC_LITE_H
#define RPC_CODEC_LITE_H

#include <memory>
#include <type_traits>
#include "tcpServer.h"
#include <zlib.h>

#include <google/protobuf/message.h>
namespace mRpc
{
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

// wire format
// Field     Length  Content
// size      4-byte  M+N+4
// tag       M-byte  could be "mRpc"
// payload   N-byte
// checksum  4-byte  adler32 of tag+payload
class ProtobufCodecLite : noncopyable
{
	public:
		const static int headerLen = sizeof(int32_t);
		const static int checksumLen = sizeof(int32_t);
		const static int maxMessageLen = 64*1024*1024; // same as codec_stream.h kDefaultTotalBytesLimit
		enum ErrorCode
		{
			noError = 0,
			invalidLength,
			checkSumError,
			invalidNameLen,
			unknownMessageType,
			parseError
		};

		typedef std::function<bool (Conn*, int32_t)> rawMessageCallback;
		typedef std::function<void (Conn*, const MessagePtr&p)> protobufMessageCallback;
		typedef std::function<void (Conn*, ErrorCode)> errorCallback;

		ProtobufCodecLite(const ::google::protobuf::Message* prototype,
						  const char * tagArg,
						  const protobufMessageCallback& messageCb,
				const rawMessageCallback& rawCb = rawMessageCallback(),
				const errorCallback& errorCb = defaultErrorCallback)
		: prototype(prototype),_tag(tagArg, strlen(tagArg)),messageCallback(messageCb),rawCb(rawCb),_errorCallback(errorCb),minMessageLen(_tag.length() + checksumLen)
		{
			LOG(INFO) << "ProtobufCodecLite Constructor" ;
		}
		virtual ~ProtobufCodecLite() { }

		const std::string& tag() const 
		{
			return _tag; 
		}
		void send(Conn* conn,const ::google::protobuf::Message& message);
		void onMessage(Conn* conn);
		bool parseFromBuffer(const unsigned char * buf, int len,google::protobuf::Message* message);
		int serializeToBuffer(const google::protobuf::Message& message,struct evbuffer* buf);
		static const std::string& errorCodeToString(ErrorCode errorCode);

		// public for unit tests
		ErrorCode parse(Conn* conn, int len, ::google::protobuf::Message* message);
		int fillEmptyBuffer(struct evbuffer* buf, const google::protobuf::Message& message);

		static int32_t checksum(struct evbuffer* buf, int len);
		static bool validateChecksum(const Bytef* buf, int len);
		static int32_t asInt32(const char* buf);
		static void defaultErrorCallback(Conn*, ErrorCode);
	private:
		const ::google::protobuf::Message* prototype;
		const std::string _tag;
		protobufMessageCallback messageCallback;
		rawMessageCallback rawCb;
		errorCallback _errorCallback;
		const uint32_t minMessageLen;
};

template<typename MSG, const char *TAG, typename CODEC=ProtobufCodecLite>  // TAG must be a variable with external linkage, not a string literal
class ProtobufCodecLiteDerived
{
	static_assert(std::is_base_of<ProtobufCodecLite, CODEC>::value, "CODEC should be derived from ProtobufCodecLite");

	public:
		typedef std::shared_ptr<MSG> ConcreteMessagePtr;
		typedef std::function<void (Conn*, const ConcreteMessagePtr&)> protobufMessageCallback;
		typedef ProtobufCodecLite::rawMessageCallback rawMessageCallback;
		typedef ProtobufCodecLite::errorCallback errorCallback;

		explicit ProtobufCodecLiteDerived(const protobufMessageCallback& messageCb,
const rawMessageCallback& rawCb = rawMessageCallback(),
const errorCallback& errorCb = ProtobufCodecLite::defaultErrorCallback)
		: messageCallback(messageCb),rpcCodeDer(&MSG::default_instance(), TAG,std::bind(&ProtobufCodecLiteDerived::onRpcMessage, this, std::placeholders::_1,std::placeholders::_2),rawCb, errorCb)
		{}

		const std::string& tag() const 
		{ 
			return rpcCodeDer.tag(); 
		}

		void send(Conn* conn,const MSG& message)
		{
			rpcCodeDer.send(conn, message);
		}

		void onMessage(Conn* conn)
		{
			rpcCodeDer.onMessage(conn);
		}

		// internal
		void onRpcMessage(Conn* conn,const MessagePtr& message)
		{
			messageCallback(conn, ::down_pointer_cast<MSG>(message));
		}

		void fillEmptyBuffer(struct evbuffer* buf, const google::protobuf::Message& message)
		{
			rpcCodeDer.fillEmptyBuffer(buf, message);
		}	
	private:
		protobufMessageCallback messageCallback;
		CODEC rpcCodeDer;

};
}
#endif  // RPC_CODEC_LITE_H__
