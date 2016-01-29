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
using namespace std;

void Read_Write_Response(int hSocket, string basedir)
{
	char pBuffer[BUFFER_SIZE];

	memset(pBuffer,0,sizeof(pBuffer));
	int rval=read(hSocket,pBuffer,BUFFER_SIZE);
	printf("Got from browser %d\n%s\n",rval,pBuffer);
	char path[MAXPATH];
	rval = sscanf(pBuffer, "GET %s HTTP/1.1", path);
	printf("Got rval %d, path %s\n",rval, path);
	char fullpath[MAXPATH];
	sprintf(fullpath, "%s%s",basedir, path);
	printf("fullpath %s\n", fullpath);

	memset(pBuffer,0,sizeof(pBuffer));
/*	sprintf(pBuffer,"HTTP/1.1 200 OK\r\n\
Content-Type: image/jpg\r\n\
Content-Length: 51793\
\r\n\r\n");
	write(hSocket,pBuffer, strlen(pBuffer));
	FILE *fp = fopen("test4.jpg","r");
	char *buffer = (char *)malloc(51793+1);
	fread(buffer, 51793, 1, fp);
	write(hSocket,buffer,51793);
	free(buffer);
	fclose(fp);
	*/
	struct stat filestat;
	char* buffer;
	string contentType = "text/html";
	int imageSize =0;
	bool image = false;
	bool error = false;
		
	if(stat(fullpath.c_str(), &filestat))
	{
		cout << "File does not exist" << endl;
		error = true;
		sprintf(pBuffer, "HTTP/1.1 404 Not Found r\nContent-Type:text/html\r\nContent-Length:0\r\n\r\n");
		write(hSocket, pBuffer, strlen(pBuffer));
		
	}
	else if(S_ISREG(filestat.st_mode))
	{
		if(path.substr(path.size()-3,path.size()-1) =="txt")
		{
			FILE *fp = fopen(path.c_str(), "r");
			buffer = (char *)malloc(filestat.st_size+1);
			memset(buffer, 0, filestat.st_size);
			fread(buffer, filestat.st_size, 1, fp);
			buffer[filestat.st_size] ='\0';
			contentType = "text/plain";
			fclose(fp);
		}
		else if (path.substr(path.size()-4,path.size()-1)=="html")
		{
			FILE *fp = fopen(path.c_str(), "r");
			buffer = (char *)malloc(filestat.st_size+1);
			memset(buffer, 0, filestat.st_size);
			fread(buffer, filestat.st_size, 1, fp);
			buffer[filestat.st_size] ='\0';
			contentType = "text/html";
			fclose(fp);

		}
		else
		{
			error = true;
			string errorMessage = "<!DOCTYPE html>\n<html><head></head>\n<body><h1>404</h1><h3>File not found.</h3></body>\n</html>";
			buffer = (char*)malloc(errorMessage.length()+1);
			memcpy(buffer, errorMessage.c_str(), errorMessage.length());
			buffer[errorMessage.length()] = '\0';
		}
	}
	else if(S_ISDIR(filestat.st_mode))
	{
		int len =0;
		DIR *dirp;
		struct dirent *dp;
		vector<string> dirList;

		dirp =opendir(path.c_str());
		while((dp = readdir(dirp)) !=NULL)
		{
			char* listing = (char*)malloc(MAX_MSG_SZ);
			sprintf(listing, "%s", dp->d_name);
			string temp = listing;
			dirList.push_back(temp);
			free(listing);
		}
		(void)closedir(dirp);
		string directoryTempString = "<html>\n<head></head>\n<body>\n";
		if (resourceStr == "/") resourceStr = "";
		for (int i = 0; i < dirList.size(); i++) {
			if (dirList[i] != "." && dirList[i] != "..") 
			{
				directoryTempString += "<a href=\"" + resourceStr + "/" + dirList[i] + "\">";
				directoryTempString += "<h4>" + dirList[i] + "</h4>";
				directoryTempString += "</a>\n";
			}
		}
		directoryTempString += "</body>\n</html>";
		buffer = (char*)malloc(directoryTempString.length()+1);
		copy(directoryTempString.begin(), directoryTempString.end(), buffer);
		buffer[directoryTempString.size()] = '\0';
		contentType = "text/html";
	}
	
	//Writing stuff
	char* headers;
	if (!image && !error)
	{
		//Create response headers
		headers = (char*)malloc(MAX_MSG_SZ);
		sprintf(headers, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %d\r\n\r\n", contentType.c_str(), strlen(buffer));
		if (write(hSocket, headers, strlen(headers)) != SOCKET_ERROR)
		{
		}
		else	
		{
			perror("Error writing response headers");
		}
		if (write(hSocket, buffer, strlen(buffer)) != SOCKET_ERROR) 
		{
		}
		else 
		{
			perror("Error writing response body");
		}
	}
	else if (error) 
	{
		//Create response headers
		headers = (char*)malloc(MAX_MSG_SZ);
		sprintf(headers, "HTTP/1.1 404 Not Found\nContent-Type: %s\nContent-Length: %d\r\n\r\n", contentType.c_str(), strlen(buffer));
		if (write(hSocket, headers, strlen(headers)) != SOCKET_ERROR) {
		}
		else
		{
			perror("Error writing response headers");
		}

		if (write(hSocket, buffer, strlen(buffer)) != SOCKET_ERROR) {
		}
		else {
			perror("Error writing response body");
		}	
	}
	else
	{
		headers = (char*)malloc(MAX_MSG_SZ);
		sprintf(headers, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %d\r\n\r\n", contentType.c_str(), imageSize);

		if (write(hSocket, headers, strlen(headers)) != SOCKET_ERROR) {
		}
		else {
			perror("Error writing response headers");
		}
		if (write(hSocket, buffer, imageSize) != SOCKET_ERROR) {
		}
		else {
			perror("Error writing response body");
		}
		free(buffer);
	}
	
	for(int i =0; i < headerLines.size(); i++)
	{
		free(headerLines[i]);
	}
	free(headers);
}

void handler (int status)
{
	printf("Received signal %d\n", status);
}
	
int main(int argc, char* argv[])
{
	int hSocket,hServerSocket;  /* handle to socket */
	struct hostent* pHostInfo;   /* holds info about a machine */
	struct sockaddr_in Address; /* Internet socket address stuct */
	int nAddressSize=sizeof(struct sockaddr_in);
	char pBuffer[BUFFER_SIZE];
	int nHostPort;
	string file_path;
	if(argc < 3)
	{
		printf("\nUsage: server host-port dir\n");
		return 0;
	}
	else
	{
		nHostPort=atoi(argv[1]);
		file_path=argv[2];
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
		Read_Write_Response(hSocket,file_path);

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
