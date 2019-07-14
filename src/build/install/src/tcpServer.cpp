#include "tcpServer.h"

using namespace::mRpc;

TcpServer::TcpServer(int count, const std::string& ip,int port):read_cb(NULL), write_cb(NULL), connect_cb(NULL), event_cb(NULL),
					thread_count(count), port(port), ip(ip), main_base(new LibeventThread),threads(new LibeventThread[thread_count])
{
	main_base->thread_id = pthread_self();
	main_base->base = event_base_new();//每个event_base结构持有一个事件集合,可以检测以确定哪个事件是激活的

	if (!main_base->base)
		throw NetErr(std::string("main thread event_base_new error:").append(evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())));

	for(int i=0;i< thread_count;i++)
		setupThread(&threads[i]);//setupThread---
	LOG(INFO) << "TcpServer structure success";
}

TcpServer::~TcpServer()
{
	quit(NULL);
	event_base_free(main_base->base);
	for(int i =0;i< thread_count;i++)
		event_base_free(threads[i].base);

	delete main_base;
	delete[] threads;
	LOG(INFO) << "TcpServer destruct success";
}

//设置相应的事件，关联事件处理函数
void TcpServer::setupThread(LibeventThread* thread)
{
	thread->tcp_server = this;
	/*
		如果设置 event_base 使用锁,则可以安全地在多个线程中访问它 
		然而,其事件循环只能 运行在一个线程中,如果需要用多个线程检测 IO,则需要为每个线程使用一个 event_base
	*/
	thread->base = event_base_new();
	if(!thread->base)
		throw NetErr(std::string("child thread event_base_new error:").append(evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())));
	
	int fds[2];
	if(pipe(fds))
		throw NetErr(std::string("child thread pipe create error:").append(strerror(errno)));
	
	thread->notify_send_fd = fds[1];
	thread->notify_recv_fd = fds[0];

	/*
	默认情况下,每当未决事件成为激活的(因为 fd 已经准备好读取或者写入,或者因为超时), 事件将在其回调被执行前成为非未决的。如果想让事件再次成为未决的 ,可以在回调函数中 再次对其调用 event_add()
	然而,如果设置了 EV_PERSIST 标志,事件就是持久的。这意味着即使其回调被激活 ,事件还是会保持为未决状态 。如果想在回调中让事件成为非未决的 ,可以对其调用 event_del ()
	每次执行事件回调的时候,持久事件的超时值会被复位。因此,如果具有 EV_READ|EV_PERSIST 标志,以及5秒的超时值,则事件将在以下情况下成为激活的:
	套接字已经准备好被读取的时候.从最后一次成为激活的开始,已经逝去 5秒
	*/
	//从管道中读数据，若可读便激活调用notifyhandler
	event_set(&thread->notify_event,thread->notify_recv_fd,EV_READ|EV_PERSIST,notifyHandler,thread);//事件被激活时调用notifyHander函数 
	event_base_set(thread->base,&thread->notify_event);//event_base_set()设置event所属的event_base
	/*
		其实在event_set()中已经将event所属的event_base设置为当前的current_base，而current_base在event_init()中被赋值为新建的event_base
		所以，如果要将一个新的event捆绑的event_base设置为新建的event_base，则可以不需要event_base_set()这步调用
		如果有多个event_base，则才需要这步；就一个event_base时，是不需要这步的，因为此时current_base就等于event_base
	*/
	if(event_add(&thread->notify_event,0) == -1)//所有新创建的事件都处于已初始化和非未决状态 ,调用 event_add()可以使其成为未决的
		throw NetErr(std::string("child thread add event error:").append(evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())));
	return;
}

void TcpServer::notifyHandler(int fd, short which,void* arg)
{
	LibeventThread * thread = (LibeventThread*)arg;
	int pipefd = thread->notify_recv_fd;

	//从管道中读数据
	evutil_socket_t confd;
	if(-1 == read(pipefd,&confd,sizeof(evutil_socket_t)))
	{
		LOG(ERROR)<<"pipe read error:"<<strerror(errno);
		return ;
	}
	if(EXIT_CODE == confd)//判断从管道中读的数据是否是退出指令
	{
		event_base_loopbreak(thread->base);
		LOG(INFO) << "notify pipe recv EXIT_CODE base loopbreak";
		return;
	}

	//为这个socket创建一个bufferevent
	struct bufferevent *bev;
	bev = bufferevent_socket_new(thread->base,confd,BEV_OPT_CLOSE_ON_FREE);
	if(!bev)
	{
		LOG(ERROR)<<"bufferevent create with evutil_socket_t  error:" <<evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
		return;
	}

	Conn *conn = thread->connect_queue.insertConn(confd,thread);

	conn->bev = bev;
	conn->thread = thread;

	bufferevent_setcb(bev,bufferReadCb,bufferWriteCb,bufferEventCb,conn);
	bufferevent_enable(bev,EV_READ|EV_WRITE);

	if(thread->tcp_server->connect_cb)
		thread->tcp_server->connect_cb(conn);

	return;
}
void TcpServer::bufferReadCb(struct bufferevent* bev,void* data)
{
	Conn* conn = (Conn*)data;
	conn->readBuf = bufferevent_get_input(bev);
	conn->writeBuf = bufferevent_get_output(bev);

	LOG(INFO)<<"have data to read";

	if(conn->getThread()->tcp_server->read_cb)
		conn->getThread()->tcp_server->read_cb(conn);

	return;
}

void TcpServer::bufferWriteCb(struct bufferevent* bev,void* data)
{
	if (!bev || !data)
		return;

	Conn* conn = (Conn*)data;
	conn->readBuf = bufferevent_get_input(bev);
	conn->writeBuf = bufferevent_get_output(bev);
	LOG(INFO)<<"have data to send";
	
	if(conn->getThread()->tcp_server->write_cb)
		conn->getThread()->tcp_server->write_cb(conn);
	return;
}
//异常
void TcpServer::bufferEventCb(struct bufferevent* bev,short events,void* data)
{
	if (events & BEV_EVENT_EOF)
	{
		LOG(INFO) <<"Connected closed."<<events;
	}
	else if (events & BEV_EVENT_READING)
	{
		LOG(INFO) <<"error when reading."<<events;
	}
	else if (events & BEV_EVENT_WRITING)
	{
		LOG(INFO) <<"error when writing."<<events;
	}
	else if (events & BEV_EVENT_TIMEOUT)
	{
		LOG(INFO) <<"time out event."<<events;
	}
	else if (events & BEV_EVENT_ERROR)
	{
		LOG(INFO) <<"error when operator."<<events;
	}
	else
	{
		LOG(INFO) <<"some unknow error!" << events;
	}

	LOG(ERROR) << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());

	Conn *conn = (Conn*)data;
	if(conn->getThread()->tcp_server->event_cb)
		conn->getThread()->tcp_server->event_cb(conn,events);

	conn->getThread()->connect_queue.deleteConn(conn);
	bufferevent_free(bev);

	return;
}

//start
void* TcpServer::threadProcess(void *arg)
{
	LibeventThread *thread = (LibeventThread*)arg;
	LOG(INFO) << "thread " << thread->thread_id << " started!";
	event_base_dispatch(thread->base);
	return NULL;
}
void TcpServer::startRun()
{
	sockaddr_in sin;
	memset(&sin,0,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if(ip != std::string())
	{
		if(inet_pton(AF_INET,ip.c_str(),&sin.sin_addr) <= 0)
			throw NetErr(std::string("sokcet ip error:").append(strerror(errno)));
	}

	//当有连接到来时调用acceptCb,socket bind listen accept
	evconnlistener *listener = evconnlistener_new_bind(main_base->base,acceptCb,(void*)this,LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,-1,(sockaddr*)&sin,sizeof(sockaddr_in));
	if(!listener)
		throw NetErr(std::string("create listener error:").append(strerror(errno)));

	//创建一批线程
	for(int i=0;i<thread_count;i++)
	{
		if(pthread_create(&threads[i].thread_id,NULL,threadProcess,(void*)&threads[i]))
			throw NetErr(std::string("pthread_create error:").append(strerror(errno)));
	}

	event_base_dispatch(main_base->base);
	evconnlistener_free(listener);
	return;
}
//当此函数被调用时，libevent已经帮我们accept了这个客户端。该客户端的,文件描述符为fd
void TcpServer::acceptCb(evconnlistener* listener, evutil_socket_t fd, sockaddr* sa, int socklen, void *user_data)
{
	TcpServer *server = (TcpServer*)user_data;
	unsigned long  num = 0;
	for (int i = 0; i< server->thread_count; i++)
	{
		num = server->threads[num].connect_queue.getCount() > server->threads[i].connect_queue.getCount() ? i : num;
	}
	//向管道中写入已连接的fd
	write(server->threads[num].notify_send_fd, &fd, sizeof(evutil_socket_t));

	return;
}

void TcpServer::quit(timeval *tv)
{
	int contant = EXIT_CODE;
	for(int i=0;i<thread_count;i++)
	{
		write(threads[i].notify_send_fd,&contant,sizeof(int));
	}
	event_base_loopexit(main_base->base,tv);

	return;
}
