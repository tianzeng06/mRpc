#include "codeClite.h"
#include "tcpServer.h"
#include "ev_endian.h"
#include "google-inl.h"

#include <google/protobuf/message.h>
#include <zlib.h>

using namespace mRpc;
using namespace mRpc::sockets;

namespace
{
	int ProtobufVersionCheck()
	{
		GOOGLE_PROTOBUF_VERIFY_VERSION;
		return 0;
	}
	int __attribute__ ((unused)) dummy = ProtobufVersionCheck();
}


namespace mRpc
{
	void ProtobufCodecLite::send(Conn* conn,const ::google::protobuf::Message& message)
	{
		struct evbuffer *ebuf = evbuffer_new();
		if (fillEmptyBuffer(ebuf, message) < 0)
		{
			LOG(ERROR) << "send error:"<<evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
		}
		if (conn->addBufToWriteBuffer(ebuf) != 0)
		{
			LOG(ERROR) << "send error:"<<evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
		}
		evbuffer_free(ebuf);
	}

	int ProtobufCodecLite::fillEmptyBuffer(struct evbuffer* buf,const google::protobuf::Message& message)
	{
		int res = 0;
		if ((res = evbuffer_add(buf, _tag.c_str(), _tag.size())) < 0)
			return res;

		int byte_size = serializeToBuffer(message, buf);

		int32_t checkSum = checksum(buf, byte_size+_tag.size());
		int32_t len = sockets::hostToNetwork32(checkSum);
		if ((res = evbuffer_add(buf, &len, sizeof len)) < 0)
			return res;
	
		len = sockets::hostToNetwork32(static_cast<int32_t>(_tag.size() + byte_size + checksumLen));
		if( (res = evbuffer_prepend(buf, &len, sizeof len)) < 0)
			return res;
		return 0;
	}

	int ProtobufCodecLite::serializeToBuffer(const google::protobuf::Message& message, struct evbuffer* buf)
	{
		GOOGLE_DCHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

		struct evbuffer_iovec vec;
		uint32_t byte_size = message.ByteSize();
		evbuffer_reserve_space(buf, byte_size+checksumLen, &vec, 1);
		uint8_t *start = static_cast<uint8_t*>(vec.iov_base);
		uint8_t *end = message.SerializeWithCachedSizesToArray(start);
		vec.iov_len = end - start;
		if ( vec.iov_len != byte_size)
		{
			ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
		}
		evbuffer_commit_space(buf, &vec, 1);

		return vec.iov_len;
	}

	void ProtobufCodecLite::onMessage(Conn* conn)
	{
		LOG(INFO) << "received message len:"<<conn->getReadBufferLen();
		while (conn->getReadBufferLen() >= static_cast<uint32_t>(minMessageLen+headerLen))
		{
			int32_t len = 0;
			conn->readBuffer(reinterpret_cast<char *>(&len), sizeof(len));
			len = asInt32(reinterpret_cast<char *>(&len));
			if (len > maxMessageLen || (uint32_t)len < minMessageLen)
			{
				_errorCallback(conn, invalidLength);/*change*/
				break;
			}
			else if (conn->getReadBufferLen()>= (static_cast<uint32_t>(len)))
			{
				if (rawCb && !rawCb(conn, len))
				{
					evbuffer_drain(conn->getReadBuffer(), len);
					continue;
				}

				MessagePtr message(prototype->New());//构造一个新的实例
				// FIXME: can we move deserialization & callback to other thread?
				ErrorCode errorCode = parse(conn, len, message.get());
				if (errorCode == noError)
				{
					// FIXME: try { } catch (...) { }
					messageCallback(conn, message);
					evbuffer_drain(conn->getReadBuffer(), len);
				}
				else
				{
					_errorCallback(conn, errorCode);
					break;
				}
			}
			else
				break;
		}
		return;
	}

	bool ProtobufCodecLite::parseFromBuffer(const unsigned char * buf, int len, google::protobuf::Message* message)
	{
		return message->ParseFromArray(buf, len);
	}

	namespace
	{
		const std::string noErrorStr("NoError");
		const std::string invalidLengthStr("InvalidLength");
		const std::string checkSumErrorStr("CheckSumError");
		const std::string invalidNameLenStr("InvalidNameLen");
		const std::string unknownMessageTypeStr("UnknownMessageType");
		const std::string parseErrorStr("ParseError");
		const std::string unknownErrorStr("UnknownError");
	}

	const std::string& ProtobufCodecLite::errorCodeToString(ErrorCode errorCode)
	{
		switch(errorCode)
		{
			case noError:
				return noErrorStr;
			case invalidLength:
				return invalidLengthStr;
			case checkSumError:
				return checkSumErrorStr;
			case invalidNameLen:
				return invalidNameLenStr;
			case unknownMessageType:
				return unknownMessageTypeStr;
			case parseError:
				return parseErrorStr;
			default:
				return unknownErrorStr;
		}
		return "";
	}

	void ProtobufCodecLite::defaultErrorCallback(Conn* conn,ErrorCode errorCode)
	{
		//LOG(DEBUF)<< "ProtobufCodecLite::defaultErrorCallback - " << errorCodeToString(errorCode);
		if (conn)
		{
			bufferevent_free(conn->getBufferevent());
			conn->getThread()->connect_queue.deleteConn(conn);
		}
		return;
	}

	int32_t ProtobufCodecLite::asInt32(const char* buf)
	{
		int32_t be32 = 0;
		::memcpy(&be32, buf, sizeof(be32));
		return sockets::networkToHost32(be32);
	}

	int32_t ProtobufCodecLite::checksum(struct evbuffer* buf, int len)
	{
		int dlen = 0;
		uLong adler = adler32(1L, Z_NULL, 0);
		int n = evbuffer_peek(buf, len, NULL, NULL, 0);
		evbuffer_iovec *vec = new evbuffer_iovec[n];
		int res = evbuffer_peek(buf, len, NULL, vec, n);
		assert( res == n);
		for (int i = 0; i < n; i++)
		{
			dlen += vec[i].iov_len;
			adler = adler32(adler, static_cast<const Bytef*>(vec[i].iov_base), vec[i].iov_len);
		}
		delete [] vec;

		return static_cast<int32_t>(adler);
	}

	bool ProtobufCodecLite::validateChecksum(const Bytef* buf, int len)
	{
		int32_t expectedCheckSum = asInt32(reinterpret_cast<const char *>(buf) + len - checksumLen);
		uLong adler = adler32(1L, Z_NULL, 0);
		adler = adler32(adler, (buf), len-checksumLen);

		return (static_cast<int32_t>(adler)) == (expectedCheckSum);
	}

	ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(Conn* conn,int len,::google::protobuf::Message* message)
	{
		ErrorCode error = noError;
		evbuffer *readBuffer = conn->getReadBuffer();
	
		unsigned char *data = evbuffer_pullup(readBuffer, len);
		if (data == NULL)
		{
			//
		}

		if(validateChecksum(static_cast<const Bytef*>(data), len))
		{
			if (memcmp(data, _tag.data(), _tag.size()) == 0)
			{
				// parse from buffer
				int32_t dataLen = len - checksumLen - static_cast<int>(_tag.size());
				if(parseFromBuffer(data+_tag.size(), dataLen,  message))
					error = noError;
			}
			else
				error = parseError;
		}
		else
			error = unknownMessageType;
		/*
		else
		{
			error = checkSumError;
		}*/
		return error;
	}
}
