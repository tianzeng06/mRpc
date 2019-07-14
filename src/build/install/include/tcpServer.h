#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glog/logging.h>

#include <stdexcept>
#include <string>
#include <functional>

#include <event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include "util.h"

namespace mRpc
{
class TcpClient;
class TcpServer;
class ConnQueue;
class Conn;
struct LibeventThread;

class NetErr : public std::runtime_error
{
public:
    NetErr(const std::string &what_arg):std::runtime_error(what_arg){}
};

//回调函数
typedef std::function<void(Conn*)> dataCallback;
typedef std::function<void(Conn*,short)> eventCallback;

//
class Conn : public noncopyable
{
    friend class ConnQueue;
    friend class TcpServer;
    friend class TcpClient;

private:
	int fd;
	//Any  context;
	LibeventThread* thread;
	bufferevent *bev;//读写缓冲区，在读出或写入足够多的数据后调用用户提供回调
	evbuffer *readBuf;
	evbuffer *writeBuf;
	Conn *pre;
	Conn *next;

public:
    Conn(int fd=0) :fd(fd),thread(NULL),bev(NULL),readBuf(NULL),
        writeBuf(NULL),pre(NULL),next(NULL) {}

    LibeventThread* getThread() 
	{
		return thread;
	}
    int getFd() 
	{
		return fd;
	}
    bufferevent* getBufferevent() 
	{
		return bev;
	}
    uint32_t getReadBufferLen()  
	{
		return evbuffer_get_length(readBuf);
	}
    evbuffer* getReadBuffer() 
	{ 
		return readBuf; 
	}
    int readBuffer(char* buffer,int len) 
	{
		return evbuffer_remove(readBuf,buffer,len);
	}
    int copyBuffer(char *buffer, int len) 
	{
		return evbuffer_copyout(readBuf, buffer, len);
	}
    uint32_t getWriteBufferLen() 
	{
		return evbuffer_get_length(writeBuf);
	}
    int addToWriteBuffer(char *buffer, int len) 
	{
		LOG(INFO)<<"add write buffer"; return evbuffer_add(writeBuf, buffer, len);
	}
    int addBufToWriteBuffer(evbuffer *buf) 
	{
		return evbuffer_add_buffer(writeBuf, buf);
	}
    void moveBufferReadToWrite() 
	{
		evbuffer_add_buffer(writeBuf, readBuf);
	}
    /*void setContext(const Any& context) 
	{
		context = context;
	}
    const Any& getContext() const 
	{
		return context;
	}
    Any* getMutableContext() 
	{
		return &context;
	}*/
};

class ConnQueue
{
private:
	unsigned long total;
	Conn* head;
	Conn* tail;

public:
    ConnQueue() : total(0), head(new Conn(0)), tail(new Conn(0))
    {
        head->pre = tail->next = NULL;
        head->next = tail; 
		tail->pre = head;
    }
    ~ConnQueue()
    {
        Conn *tcur=head,*tnext=NULL;
        while(tcur != NULL) 
		{
            tnext = tcur->next;
            delete tcur;
            tcur = tnext;
        }
    }

    unsigned long getCount() 
	{
		return total;
	}

    Conn* insertConn(int fd,LibeventThread* t)
    {
        Conn *c = new Conn(fd);
        c->thread = t;
        Conn *next = head->next;

        c->pre = head;
        c->next = head->next;
        head->next = c;
        next->pre = c;
        total++;
        return c;
    }
    void deleteConn(Conn* conn)
    {
        conn->pre->next = conn->next;
        conn->next->pre = conn->pre;
        delete conn;
        total--;
    }
};

//封装线程id和相应事件...
struct LibeventThread
{
	pthread_t thread_id;
	struct event_base *base;
	struct event notify_event;
	int notify_send_fd;
	int notify_recv_fd;

	ConnQueue connect_queue;
	union
	{
		TcpServer *tcp_server;
		TcpClient *tcp_client;
	};
	LibeventThread() :thread_id(0), notify_send_fd(0), notify_recv_fd(0) {};
};

class TcpServer: public noncopyable
{
private:
	void setupThread(LibeventThread* thread);
	static void* threadProcess(void *arg);
	static void notifyHandler(int fd, short which, void* arg);
	static void acceptCb(evconnlistener* listener, evutil_socket_t fd, sockaddr* sa, int socklen, void *user_data);

	static void bufferReadCb(struct bufferevent* bev, void* data);
	static void bufferWriteCb(struct bufferevent* bev, void* data);
	static void bufferEventCb(struct bufferevent* bev, short events, void* data);

	dataCallback read_cb;//读回调
	dataCallback write_cb;//写回调
	dataCallback connect_cb;//连接回调
	eventCallback event_cb;//错误事件连接回调

	int thread_count;
	int port;
	std::string ip;
	LibeventThread* main_base;
	LibeventThread* threads;

public:
    static const int EXIT_CODE = -1;

    TcpServer(int count, const std::string& ip,int port = 0);
    ~TcpServer();
    void startRun();
    void quit(timeval *tv);

    // read enough data then call ReadCallback
    void setReadCallback(const dataCallback& cb)
    {
        read_cb =cb;
    }
    // write enough data the call WriteCallback
    void setWriteCallback(const dataCallback& cb)
    {
        write_cb = cb;
    }
    // add a new conn then call ConnectCallback
    void setConnectionCallback(const dataCallback& cb)
    {
        connect_cb = cb;
    }
    // error then call eventCallback
    void setEventCallback(const eventCallback& cb)
    {
        event_cb = cb;
    }
};
}
#endif // EKV_SERVER_H
