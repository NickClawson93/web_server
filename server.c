#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <dirent.h>


#define SOCKET_ERROR        -1
#define BUFFER_SIZE         10000
#define MESSAGE             "This is the message I'm sending back and forth"
#define QUEUE_SIZE          5
#define DIRECTORY_LENGTH    100

using namespace std;


string parse_request(string request){
    cout << endl;
    stringstream ss;
    ss << request;    
    string path;
    string temp;
    ss >> temp;
    cout << temp << endl;
    if (temp != "GET") {
        return "FAILED";
    }
    ss >> path;
    cout << path << endl;
    ss >> temp;
    cout << temp << endl;
    if (temp != "HTTP/1.1"){
        return "FAILED";
    }
    return path;
}

string get_extension(string filename){
    return filename.substr(filename.find_last_of("."));
}

string generate_headers(string file_extension, int file_size){
    string headers = "";
    map<string, string> content_types;
    content_types[".html"] = "text/html";
    content_types[".txt"] =  "text/plain";
    content_types[".jpg"] = "image/jpg";
    content_types[".gif"] = "image/gif";
    ostringstream oss;
    oss << "Content-Length: " << file_size << "\r\n";
    oss << "Content-Type: " << content_types.at(file_extension) << "\r\n";
    oss << "\r\n";
    return oss.str();
}

string generate_response(string path){
    string response = "";
//Stat code    
    struct stat filestat;

    if(stat(path.c_str(), &filestat)) {
        cout <<"ERROR in stat\n";
        response = "HTTP/1.1 404 NOT FOUND\r\n\r\n404 Not Found\n";
        return response;
    }
    response = "HTTP/1.1 200 OK";
    if(S_ISREG(filestat.st_mode)) {
        cout << path << " is a regular file \n";
        cout << "file size = "<<filestat.st_size <<"\n";
        FILE *fp = fopen(path.c_str(), "r");
        char *buffer = (char *) malloc(filestat.st_size + 1);
        fread(buffer, filestat.st_size, 1, fp);
        printf("read\n%s", buffer);
        response += generate_headers(get_extension(path), filestat.st_size);
        response += (string)buffer;
        free(buffer);
        fclose(fp);
    }
    if(S_ISDIR(filestat.st_mode)) {
        cout << path << " is a directory \n";
        DIR *dirp;
        struct dirent *dp;
        dirp = opendir(path.c_str());
        string content = "<html><ul>";
        while ((dp = readdir(dirp)) != NULL)
            cout << "name " << dp ->d_name << endl;
            content += "<li>" + (string)dp->d_name + "</li>";
        content += "</ul></html>";
        response += generate_headers(".html", content.length());
        response += content;
        (void)closedir(dirp);
    }
//end stat code
    return response;
}

int main(int argc, char* argv[])
{
    int hSocket,hServerSocket;  /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize=sizeof(struct sockaddr_in);
    char pBuffer[BUFFER_SIZE];
    int nHostPort;
    char base_directory[DIRECTORY_LENGTH];
    if(argc < 3)
      {
        printf("\nUsage: server host-port directory\n");
        return 0;
      }
    else
      {
        nHostPort=atoi(argv[1]);
        strcpy(base_directory, argv[2]);
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
    if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address)) 
                        == SOCKET_ERROR)
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
    int optval = 1;
    setsockopt(hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    for(;;)
    {
        printf("\nWaiting for a connection\n");
        /* get the connected socket */
        hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);

        printf("\nGot a connection from %X (%d)\n",
              Address.sin_addr.s_addr,
              ntohs(Address.sin_port));
        memset(pBuffer,0,sizeof(pBuffer));
        read(hSocket,pBuffer,BUFFER_SIZE);
        printf("got from browser \n%s\n", pBuffer);
        string path = parse_request((string) pBuffer);
        string response = generate_response((string)base_directory + path);
        cout << "------------response----------\n" + response + "-----------end response -------" << endl;
        printf("\nClosing the socket");
        memset(pBuffer,0,sizeof(pBuffer));
        sprintf(pBuffer, "HTTP/1.1 200 OK\r\n\r\n<html>Hello</html>\r\n\r\n");
        write(hSocket, response.c_str(), response.length());
//linger
//shutdown
        /* close socket */
        if(close(hSocket) == SOCKET_ERROR)
        {
         printf("\nCould not close socket\n");
         return 0;
        }
    }
}

