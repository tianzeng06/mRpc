#ifndef RPC_TCPCLIENT_H
#define RPC_TCPCLIENT_H

#include "tcpServer.h"

namespace mRpc
{


class TcpClient: public noncopyable
{
public:

    TcpClient(const std::string& ip,int port = 0);
    ~TcpClient();

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

private:
    static void* threadProcess(void *arg);
    static void notifyHandler(int fd,short which,void* arg);


    static void acceptCb(evconnlistener* listener,evutil_socket_t fd,sockaddr* sa,int socklen,void *user_data);

    static void bufferReadCb(struct bufferevent* bev,void* data);
    static void bufferWriteCb(struct bufferevent* bev,void* data);
    static void bufferEventCb(struct bufferevent* bev,short events,void* data);

    dataCallback read_cb;
    dataCallback write_cb;
    dataCallback connect_cb;
    eventCallback event_cb;

    int port;
    std::string ip;
    Conn        conn_;
    LibeventThread main_base;
};


}
#endif // TCPCLIENT_H
