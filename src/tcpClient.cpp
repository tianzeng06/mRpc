#include "tcpClient.h"
#include "tcpServer.h"
#include <errno.h>

using namespace::mRpc;

TcpClient::TcpClient(const std::string& ip,int port)
  :read_cb(NULL), write_cb(NULL), connect_cb(NULL), event_cb(NULL),
    port(port), ip(ip), main_base()
{
  main_base.thread_id = pthread_self();
  conn_.thread = &main_base;
  main_base.tcp_client = this;
  main_base.base = event_base_new();
  if (!main_base.base)
    throw NetErr(std::string("Client event_base_new error:").append(
                      evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())));

  LOG(INFO) << "TcpClient init success";
}

TcpClient::~TcpClient()
{
  quit(NULL);
  event_base_free(main_base.base);
  LOG(INFO)<< "TcpClient dtor";
}

void TcpClient::bufferReadCb(struct bufferevent* bev,void* data)
{
  Conn* conn = (Conn*)data;
  conn->readBuf = bufferevent_get_input(bev);
  conn->writeBuf = bufferevent_get_output(bev);

  LOG(INFO)<<"have data to read";

  if(conn->getThread()->tcp_client->read_cb)
    conn->getThread()->tcp_client->read_cb(conn);
}
void TcpClient::bufferWriteCb(struct bufferevent* bev,void* data)
{
  Conn* conn = (Conn*)data;
  conn->readBuf = bufferevent_get_input(bev);
  conn->writeBuf = bufferevent_get_output(bev);
  LOG(INFO)<<"have data to send";
  if(conn->getThread()->tcp_client->write_cb)
    conn->getThread()->tcp_client->write_cb(conn);
}
void TcpClient::bufferEventCb(struct bufferevent* bev,short events,void* data)
{
  Conn *conn = (Conn*)data;

  if (events & BEV_EVENT_EOF){
    LOG(INFO) <<"Connected closed."<<events;
  }else if (events & BEV_EVENT_READING){
    LOG(INFO) <<"error when reading."<<events;
  }else if (events & BEV_EVENT_WRITING){
    LOG(INFO) <<"error when writing."<<events;
  }else if (events & BEV_EVENT_TIMEOUT){
    LOG(INFO) <<"time out event."<<events;
  }else if (events & BEV_EVENT_ERROR){
    LOG(INFO) <<"error when operator."<<events;
  }else if (events & BEV_EVENT_CONNECTED){
    LOG(INFO) <<"connected success."<<events;
    conn->fd = bufferevent_getfd(bev);
    conn->readBuf = bufferevent_get_input(bev);
    conn->writeBuf = bufferevent_get_output(bev);

    if(conn->getThread()->tcp_client->connect_cb)
      conn->getThread()->tcp_client->connect_cb(conn);
    return;
//  }else if (EVUTIL_SOCKET_ERROR() == 115){
//    LOG(INFO) <<"info::::::is not connected."<<events;
//    return;
  }else{
    LOG(INFO) <<"some unknow error!" << events;
  }

  LOG(ERROR) <<EVUTIL_SOCKET_ERROR()<< "\t" << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
  if(conn->getThread()->tcp_client->event_cb)
    conn->getThread()->tcp_client->event_cb(conn, events);

  bufferevent_free(bev);
}
void TcpClient::startRun()
{  
  sockaddr_in sin;
  memset(&sin,0,sizeof(sin));

  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  if(ip != std::string())
  {
    if(inet_pton(AF_INET,ip.c_str(),&sin.sin_addr) <= 0)
      throw NetErr("sokcet ip error!");
  }
  errno = 0;
  conn_.bev = bufferevent_socket_new(this->main_base.base, -1, BEV_OPT_CLOSE_ON_FREE);
  if (!conn_.bev){
    throw NetErr(std::string("New bufferevent socket error:").append(
                      evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())));
  }

  bufferevent_setcb(conn_.bev,bufferReadCb,bufferWriteCb,bufferEventCb, &conn_);
  bufferevent_enable(conn_.bev, EV_WRITE|EV_READ);

  if ((bufferevent_socket_connect(conn_.bev, (struct sockaddr *)(&sin), sizeof(sin))) < 0)
  {
    bufferevent_free(conn_.bev);
    LOG(ERROR) << "connect to " << ip << "error :\n\t"
               <<evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
    return;
  }

//  conn_->readBuf = bufferevent_get_input(conn_.bev);
//  conn_->writeBuf = bufferevent_get_output(conn_.bev);
  event_base_dispatch(main_base.base);

//  printf("Finished\n");
}

void TcpClient::quit(timeval *tv)
{
  bufferevent_free(conn_.bev);
  event_base_loopexit(main_base.base,tv);
}

