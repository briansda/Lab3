#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "parse.h"

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         10000
#define QUEUE_SIZE          5
#define MAX_MSG_SZ 1024
#define HOST_NAME_SIZE 255
#define MAXPATH 1000
	
	int hSocket,hServerSocket;  /* handle to socket */
	struct hostent* pHostInfo;   /* holds info about a machine */
	struct sockaddr_in Address; /* Internet socket address stuct */
	int nAddressSize=sizeof(struct sockaddr_in);
	char pBuffer[BUFFER_SIZE];
	int nHostPort;
	char *cleint_request;
	string directorypath;
	char _directorypath[MAXPATH];
	

using namespace std;
//Check to see what file type it is
string Type (string path)
{
	string type = path.substr(path.size() -4,4);
	string output ="";
	if(type=="html")
	{
		output= "text/html";
	}
	else if (type ==".txt")
	{
		output="text/plain";
	}
	else if (type ==".jpg")
	{
		output="image/jpg";
	}
	else if (type == ".gif")
	{
		output="image/gif";
	}
	else
	{
		//just put it as html to see what happens if it isn't one of the others
		output="text/html";
	}
	return output;
}
//check to see how long of a file it is
long Length(string path)
{
	struct stat thing;
	int rc =stat(path.c_str(),&thing);
	if(rc==0)
	{
		return thing.st_size;
	}
	else
	{
		return -1;
	}
}
//Make an index file html doc
string MakeIndex(string root, string path)
{
	string index = "<h1>" + path + " Index </h1><ul>";
	int len;
	DIR *dirp;
	struct dirent *dp;
	
	dirp = opendir((root +path).c_str());
	if (path != "/")
	{
		 path += "/";
	}
	while ((dp = readdir(dirp)) != NULL)
	{
		string file = string(dp->d_name);
		if(file != "." && file != "..")
		{
			index += "<li><a href='"+ path + file +"'>"+ file + "</li>";
		}
	}
	index += "</ul>";
	string output = "<!DOCTYPE html><html>" + index + "</html>";
	(void)closedir(dirp);
	return output;
}

void handler (int status)
{
	printf("Received signal %d\n", status);
}
	
void Read_Write_Response(int hSocket)
{
	//Clears memory
	memset(pBuffer,0,sizeof(pBuffer));
	//places read val
	int rval=read(hSocket,pBuffer,BUFFER_SIZE);
	//gets path from read value
	char path[MAXPATH];
	rval = sscanf(pBuffer, "GET %s HTTP/1.1", path);
	//adds home directory to it to make fullpath to it
	char fullpath[MAXPATH];
	sprintf(fullpath, "%s%s",_directorypath, path);
	//Makes variable name for fullpath to use
	const string& _fullpath=fullpath;

	//clears memory	
	memset(pBuffer,0,sizeof(pBuffer));
	//creates stat structure for use, buffer, and variable content type
	struct stat filestat;
	char* buffer;
	string contentType = "text/html";
	
	//Does file exist, if not, produce error	
	if(stat(_fullpath.c_str(), &filestat)!=0)
	{
		string errorMessage = "<!DOCTYPE html>\n<html><head></head>\n<body><h1>404</h1><h3>File not found.</h3></body>\n</html>";
		sprintf(pBuffer, "HTTP/1.1 404 Not Found\r\nContent-Type:text/html\r\nContent-Length:%ld\r\n\r\n", errorMessage.length());
		write(hSocket, pBuffer, strlen(pBuffer));
		write(hSocket, errorMessage.c_str(), strlen(errorMessage.c_str()));	
	}
	//If it is a directory treat it special
	else if(S_ISDIR(filestat.st_mode))
	{
		//check if it has an index.html, if not make one
		string indexpath= _fullpath + "/index.html";
		if (stat(indexpath.c_str(), &filestat)==0)
		{
			sprintf(path, "%s/index.html", path);
			sprintf(fullpath, "%s/index.html", fullpath);
			contentType = Type(path);
			long contentLength = Length(fullpath);
			sprintf(pBuffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",contentType.c_str(),contentLength);
			write(hSocket, pBuffer, strlen(pBuffer));
			FILE *fp = fopen(fullpath, "r");
			char *buffer = (char *)malloc( contentLength+ 1);
			fread(buffer, contentLength, 1, fp);
			write(hSocket, buffer, contentLength);
			free(buffer);
			fclose(fp);
		}
		else
		{
			cout << "Client requested a folder without an index." << endl;
			string index = MakeIndex(directorypath, path);
			sprintf(pBuffer, "HTTP:/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld \r\n\r\n", index.length());
			write(hSocket, pBuffer,strlen(pBuffer));
			write(hSocket, index.c_str(), strlen(index.c_str()));
		}
	
	}
	//If it is a regular file treat it normal
	else if(S_ISREG(filestat.st_mode))
	{
		string contentType =Type(path);
		long contentLength = Length(fullpath);
		sprintf(pBuffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", contentType.c_str(), contentLength);
		write(hSocket, pBuffer, strlen(pBuffer));
		FILE *fp =fopen(fullpath, "r");
		char *buffer = (char *)malloc(contentLength +1);
		fread(buffer, contentLength, 1, fp);
		write(hSocket, buffer, contentLength);
		free(buffer);
		fclose(fp);
	}
}

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		printf("\nUsage: server host-port dir\n");
		return 0;
	}
	else
	{
		nHostPort=atoi(argv[1]);
		directorypath=argv[2];
		strcpy(_directorypath, argv[2]);
	}

	printf("\nStarting server");
	printf("\nMaking socket");
	/* make a socket */
	hServerSocket=socket(AF_INET,SOCK_STREAM,0);
	if(hServerSocket == SOCKET_ERROR)
	{
		printf("\nCould not make a socket\n");
		return 0;
	}
	/* fill address struct */
	Address.sin_addr.s_addr=INADDR_ANY;
	Address.sin_port=htons(nHostPort);
	Address.sin_family=AF_INET;
	printf("\nBinding to port %d",nHostPort);

	/* bind to a port */
	if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address)) == SOCKET_ERROR)
	{
		printf("\nCould not connect to host\n");
		return 0;
	}
	/*  get port number */
	getsockname( hServerSocket, (struct sockaddr *) &Address,(socklen_t *)&nAddressSize);
	printf("opened socket as fd (%d) on port (%d) for stream i/o\n",hServerSocket, ntohs(Address.sin_port) );
	printf("Server\n\
		sin_family        = %d\n\
		sin_addr.s_addr   = %d\n\
		sin_port          = %d\n"
		, Address.sin_family
		, Address.sin_addr.s_addr
		, ntohs(Address.sin_port)
		);

	printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);
	/* establish listen queue */
	if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
	{
		printf("\nCould not listen\n");
		return 0;
	}
	int optval =1;
	setsockopt (hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	//Prevent killing by pipe or
	struct sigaction sigold, signew;
	signew.sa_handler=handler;
	sigemptyset(&signew.sa_mask);
	sigaddset(&signew.sa_mask, SIGINT);
	signew.sa_flags=SA_RESTART;
	sigaction(SIGHUP,&signew,&sigold);
	sigaction(SIGPIPE,&signew,&sigold);

	for(;;)
	{
		printf("\nWaiting for a connection\n");
		/* get the connected socket */
		hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);
		printf("\nGot a connection from %X (%d)\n", Address.sin_addr.s_addr, ntohs(Address.sin_port));
		Read_Write_Response(hSocket);

		#ifdef notdef
		linger lin;
		unsigned int y=sizeof(lin);
		lin.1_onoff=1;
		lin.l_linger=10;
		setsockopt(hSocket,SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
		shutdown(hSocket, SHUT_RDWR);
		#endif
		printf("\nClosing the socket");	
		/* close socket */
		if(close(hSocket) == SOCKET_ERROR)
		{
			printf("\nCould not close socket\n");
			return 0;
		}
	}
}
