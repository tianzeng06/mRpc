#include<iostream>
#include "tcpserver.h"
#include <assert.h>
#include <thread>

using namespace std;
using namespace mRpc;

class TcpServerImpl : noncopyable
{
	public:
		TcpServerImpl() {}
		~TcpServerImpl() {}

		static void onMessage(Conn *conn)
		{
			char buffer[1024] = {};
			int len=0, res = 0;
			len = conn->getReadBufferLen();

			evbuffer *evbuf= conn->getReadBuffer();
			int n = evbuffer_peek(evbuf, len, NULL, NULL, 0);
			evbuffer_iovec *vec = new evbuffer_iovec[n];
			res = evbuffer_peek(evbuf, len, NULL, vec, n);
			assert( res == n);

			for (int i = 0; i < n; i++)
			{
				string str((char *)(vec[i].iov_base), vec[i].iov_len);
				cout <<"client: "<< str << endl;
			}
			delete [] vec;

			for(int i =0; i< len/1024+1;i++)
			{
				res = conn->readBuffer(buffer, (i*1023)>len ? (i%1023):1023);
				cout<< "\t\t\tbuf " << i << ":" << buffer << endl;
			}
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}

		static void send(Conn *conn)
		{
			string str("This is my Rpc");
			conn->addToWriteBuffer(const_cast<char *>(str.c_str()), str.size());

			std::this_thread::sleep_for(std::chrono::seconds(3));
		}

		static void onAccept(Conn *conn)
		{
			cout << "client connect:"<<endl;
			cout << "fd:" << conn->getFd() << "\tthread id:" << conn->getThread()->thread_id << endl;
		}

		static void onError(Conn *conn, short event)
		{
			cout << "client closed : " << endl;
			cout << "fd:" << conn->getFd() << "\tthread id:" << conn->getThread()->thread_id << endl;
		}
};

int main()
{
	TcpServer server(4, "127.0.0.1", 8009);
	server.setConnectionCallback(&TcpServerImpl::onAccept);
	server.setReadCallback(&TcpServerImpl::onMessage);
	server.setWriteCallback(&TcpServerImpl::send);
	server.setEventCallback(&TcpServerImpl::onError);
	server.startRun();
	return 0;
}
