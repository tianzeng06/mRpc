#include "rpcCode.h"
#include "tcpServer.h"
#include "rpc.pb.h"
#include "google-inl.h"

namespace
{
	int ProtobufVersionCheck()
	{
		GOOGLE_PROTOBUF_VERIFY_VERSION;
		return 0;
	}
	int dummy __attribute__ ((unused)) = ProtobufVersionCheck();
}

namespace mRpc
{
	const char rpctag [] = "mRpc";
}
