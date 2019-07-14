#include "tcpServer.h"

using namespace::mRpc;

TcpServer::TcpServer(int count, const std::string& ip,int port):read_cb(NULL), write_cb(NULL), connect_cb(NULL), event_cb(NULL),
					thread_count(count), port(port), ip(ip), main_base(new LibeventThread),threads(new LibeventThread[thread_count])
{
	main_base->thread_id = pthread_self();
	main_base->base = event_base_new();//ÿ��event_base�ṹ����һ���¼�����,���Լ����ȷ���ĸ��¼��Ǽ����

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

//������Ӧ���¼��������¼�������
void TcpServer::setupThread(LibeventThread* thread)
{
	thread->tcp_server = this;
	/*
		������� event_base ʹ����,����԰�ȫ���ڶ���߳��з����� 
		Ȼ��,���¼�ѭ��ֻ�� ������һ���߳���,�����Ҫ�ö���̼߳�� IO,����ҪΪÿ���߳�ʹ��һ�� event_base
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
	Ĭ�������,ÿ��δ���¼���Ϊ�����(��Ϊ fd �Ѿ�׼���ö�ȡ����д��,������Ϊ��ʱ), �¼�������ص���ִ��ǰ��Ϊ��δ���ġ���������¼��ٴγ�Ϊδ���� ,�����ڻص������� �ٴζ������ event_add()
	Ȼ��,��������� EV_PERSIST ��־,�¼����ǳ־õġ�����ζ�ż�ʹ��ص������� ,�¼����ǻᱣ��Ϊδ��״̬ ��������ڻص������¼���Ϊ��δ���� ,���Զ������ event_del ()
	ÿ��ִ���¼��ص���ʱ��,�־��¼��ĳ�ʱֵ�ᱻ��λ�����,������� EV_READ|EV_PERSIST ��־,�Լ�5��ĳ�ʱֵ,���¼�������������³�Ϊ�����:
	�׽����Ѿ�׼���ñ���ȡ��ʱ��.�����һ�γ�Ϊ����Ŀ�ʼ,�Ѿ���ȥ 5��
	*/
	//�ӹܵ��ж����ݣ����ɶ��㼤�����notifyhandler
	event_set(&thread->notify_event,thread->notify_recv_fd,EV_READ|EV_PERSIST,notifyHandler,thread);//�¼�������ʱ����notifyHander���� 
	event_base_set(thread->base,&thread->notify_event);//event_base_set()����event������event_base
	/*
		��ʵ��event_set()���Ѿ���event������event_base����Ϊ��ǰ��current_base����current_base��event_init()�б���ֵΪ�½���event_base
		���ԣ����Ҫ��һ���µ�event�����event_base����Ϊ�½���event_base������Բ���Ҫevent_base_set()�ⲽ����
		����ж��event_base�������Ҫ�ⲽ����һ��event_baseʱ���ǲ���Ҫ�ⲽ�ģ���Ϊ��ʱcurrent_base�͵���event_base
	*/
	if(event_add(&thread->notify_event,0) == -1)//�����´������¼��������ѳ�ʼ���ͷ�δ��״̬ ,���� event_add()����ʹ���Ϊδ����
		throw NetErr(std::string("child thread add event error:").append(evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())));
	return;
}

void TcpServer::notifyHandler(int fd, short which,void* arg)
{
	LibeventThread * thread = (LibeventThread*)arg;
	int pipefd = thread->notify_recv_fd;

	//�ӹܵ��ж�����
	evutil_socket_t confd;
	if(-1 == read(pipefd,&confd,sizeof(evutil_socket_t)))
	{
		LOG(ERROR)<<"pipe read error:"<<strerror(errno);
		return ;
	}
	if(EXIT_CODE == confd)//�жϴӹܵ��ж��������Ƿ����˳�ָ��
	{
		event_base_loopbreak(thread->base);
		LOG(INFO) << "notify pipe recv EXIT_CODE base loopbreak";
		return;
	}

	//Ϊ���socket����һ��bufferevent
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
//�쳣
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

	//�������ӵ���ʱ����acceptCb,socket bind listen accept
	evconnlistener *listener = evconnlistener_new_bind(main_base->base,acceptCb,(void*)this,LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,-1,(sockaddr*)&sin,sizeof(sockaddr_in));
	if(!listener)
		throw NetErr(std::string("create listener error:").append(strerror(errno)));

	//����һ���߳�
	for(int i=0;i<thread_count;i++)
	{
		if(pthread_create(&threads[i].thread_id,NULL,threadProcess,(void*)&threads[i]))
			throw NetErr(std::string("pthread_create error:").append(strerror(errno)));
	}

	event_base_dispatch(main_base->base);
	evconnlistener_free(listener);
	return;
}
//���˺���������ʱ��libevent�Ѿ�������accept������ͻ��ˡ��ÿͻ��˵�,�ļ�������Ϊfd
void TcpServer::acceptCb(evconnlistener* listener, evutil_socket_t fd, sockaddr* sa, int socklen, void *user_data)
{
	TcpServer *server = (TcpServer*)user_data;
	unsigned long  num = 0;
	for (int i = 0; i< server->thread_count; i++)
	{
		num = server->threads[num].connect_queue.getCount() > server->threads[i].connect_queue.getCount() ? i : num;
	}
	//��ܵ���д�������ӵ�fd
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
