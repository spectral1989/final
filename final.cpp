#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <string>     // std::string, std::stoi
#include "http_parser.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <thread>
#include <arpa/inet.h>
#include <signal.h>
// #include <sys/net.h>

// #include <ev.h>
using namespace std;


 typedef struct {
  //socket_t sock;
  string filename;
  int buf_len;
 } custom_data_t;


int my_url_callback(http_parser* p, const char *at, size_t length)
{
	custom_data_t *custom_data = (custom_data_t *)p->data;

	char cgi[] = "?";
	char temp[1000];
	int i;
	for(i = 1; i < 1000; i++)
	{
		if(at[i] == cgi[0])
		{
			temp[i - 1] = 0;
			break;
		}
		else
			temp[i - 1] = at[i];
	}
//	i++;
	string str(temp, i);
	custom_data->filename = str;

	return 0;
}



void threadFunction()
{
	while(1)
		sleep(1);
}

static const char* templ = "HTTP/1.0 200 OK\r\n"

		           "Content-length: %d\r\n"

		       	   // "Connection: close\r\n"

		       	   "Content-Type: text/html\r\n"

		       	   "\r\n"

		       	   "%s";


static const char not_found[] = "HTTP/1.0 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n";




int main(int argc, char *argv[])
{
	int port = 5000;
	char filename[100] = {};
	char ip[20] = {};
	int opt;
	char optString;
	int nparsed = 0;

	opt = getopt( argc, argv, "h:p:d:" );
    while( opt != -1 )
    {
        switch( opt ) {
            case 'h':
                // ip = atoi(optarg);
            	memcpy(ip, optarg, 19);
            	cout << ip << endl;
                break;

            case 'p':
            	port = atoi(optarg);
            	cout << port << endl;
                break;

            case 'd':
                memcpy(filename, optarg, 99);
                break;
        }

        opt = getopt( argc, argv, "h:p:d:" );
    }
    int i = 0;
    char tt[] = "/";
    while(i++ < 100)
    {
    	if(filename[i] == 0)
    	{
    		if(filename[i-1] != tt[0])
    			filename[i] = tt[0];
    		break;
    	}
    }
    cout << filename << endl;

    pid_t parpid;

	if((parpid=fork())<0) //--здесь мы пытаемся создать дочерний процесс главного процесса (масло масляное в прямом смысле)
	{                   //--точную копию исполняемой программы
		printf("\ncan't fork"); //--если нам по какойлибо причине это сделать не удается выходим с ошибкой.
		exit(1);                //--здесь, кто не совсем понял нужно обратится к man fork
	}
	else if (parpid!=0) //--если дочерний процесс уже существует
		exit(0);            //--генерируем немедленный выход из программы(зачем нам еще одна копия программы)
	signal(SIGHUP, SIG_IGN);
	setsid();           //--перевод нашего дочернего процесса в новую сесию

	std::thread thr(threadFunction);


    http_parser_settings settings = {};
	settings.on_url = my_url_callback;

	http_parser *parser = (http_parser *)malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);


	char raw[] = "GET /testname HTTP/1.1\r\n"
         "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"
         "Host: 0.0.0.0=5000\r\n"
         "Accept: */*\r\n"
         "\r\n";
     size_t raw_len = strlen(raw);

    printf("Listening on port %d\n", port);

    struct sockaddr_in addr;
    struct sockaddr_in client;

    int s = socket(PF_INET, SOCK_STREAM, 0);

    int enable = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	    perror("setsockopt(SO_REUSEADDR) failed");

    addr.sin_family = AF_INET;
    addr.sin_port     = htons(port);
//    inet_pton(AF_INET, ip, &addr);
    addr.sin_addr.s_addr = inet_addr(ip);//INADDR_ANY;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            perror("bind");
    }

    // fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);

    listen(s, SOMAXCONN);

    int connfd = 0;
    socklen_t size;

    while(1)
    {
    	/* allocate  for user data */
 		custom_data_t *my_data = new custom_data_t;
 		parser->data = my_data;

        connfd = accept(s, (struct sockaddr*)&client, &size);

        if(connfd < 0)
        {
        	cout << "error " << endl;
        	return -1;
        }
        cout << "client connected " << size << endl;

        char buf[1000], sendbuf[1000];
        memset(buf, 0, sizeof(buf) - 1);
        ssize_t recv_size = recv(connfd, buf, 1000, MSG_NOSIGNAL);

        cout << "readed: " << recv_size << endl;

        /* Start up / continue the parser.
		 * Note we pass recved==0 to signal that EOF has been received.
		 */
		nparsed = http_parser_execute(parser, &settings, buf, recv_size);

		cout << "filename: " << my_data->filename.c_str() << endl;

		memset(buf, 0, sizeof(buf) - 1);
		strcpy (buf,filename);
		strcat (buf,my_data->filename.c_str());
		int fd = open(buf, O_RDONLY);

		if(fd > 0)
		{
			memset(buf, 0, sizeof(buf) - 1);
			ssize_t rd = read(fd, buf, 1000);
			cout << "readed: " << rd << " " << buf << endl;
//			strcat (buf,filename);
//			strcpy (buf,filename);
			sprintf(sendbuf, templ, rd, buf);
			cout << "send: " << sendbuf << endl;
			send(connfd, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
		}
		else
        	send(connfd, not_found, strlen(not_found), MSG_NOSIGNAL);

//		sleep(1);
        close(connfd);

        delete my_data;
        //
    }

	if (parser->upgrade) {
	  /* handle new protocol */
		cout << "handle new protocol" << endl;
	} else if (nparsed != raw_len) {
	  /* Handle error. Usually just close the connection. */
		cout << "Handle error" << endl;
	}


	return 0;
}
