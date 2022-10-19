#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"
#include <fcntl.h>
#include "msg.h"

void Usage(char *progname);
void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);
void PrintReverseDNS(struct sockaddr *addr, size_t addrlen);
void PrintServerSide(int client_fd, int sock_family);
int  Listen(char *portnum, int *sock_family);
void HandleClient(int c_fd, struct sockaddr *addr, size_t addrlen,
                  int sock_family);
void *tree(void *arg);
int adds(int32_t fd,struct msg s);
struct msg get(int32_t fd,int in);
int del(int32_t fd,int in);


//information we need to pass to the tree/handeler
struct info {
   int fd;
   int number;
   
 
};





int 
main(int argc, char **argv) {
  // Expect the port number as a command line argument.
  if (argc != 2) {
    Usage(argv[0]);
  }

  int sock_family;
  int listen_fd = Listen(argv[1], &sock_family);
  if (listen_fd <= 0) {
    // We failed to bind/listen to a socket.  Quit with failure.
    printf("Couldn't bind to any addresses.\n");
    return EXIT_FAILURE;
  }

  // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.

struct info info;
    info.fd=0;
    info.number=0;

    pthread_t p[5000];
    int i=0;

  while (1) {
   //file information
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    //accepts queued clients
    int client_fd = accept(listen_fd,
                           (struct sockaddr *)(&caddr),
                           &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      printf("Failure on accept:%s \n ", strerror(errno));
      break;
    }
    //gets numbers needed for the structure
info.fd = client_fd;
info.number = i;
 //makes thread
  Pthread_create(&p[i], NULL, tree, &info);
  
  i++;
  
  
   

  }

  // Close socket
  close(listen_fd);
  return EXIT_SUCCESS;
}













void Usage(char *progname) {
  printf("usage: %s port \n", progname);
  exit(EXIT_FAILURE);
}

void 
PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  printf("Socket [%d] is bound to: \n", fd);
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    printf(" IPv4 address %s", astring);
    printf(" and port %d\n", ntohs(in4->sin_port));

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    printf("IPv6 address %s", astring);
    printf(" and port %d\n", ntohs(in6->sin6_port));

  } else {
    printf(" ???? address and port ???? \n");
  }
}

void 
PrintReverseDNS(struct sockaddr *addr, size_t addrlen) {
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, addrlen, hostname, 1024, NULL, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
  }
  printf("DNS name: %s \n", hostname);
}

void 
PrintServerSide(int client_fd, int sock_family) {
  char hname[1024];
  hname[0] = '\0';

  printf("Server side interface is ");
  if (sock_family == AF_INET) {
    // The server is using an IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  }
}

int 
Listen(char *portnum, int *sock_family) {

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Use argv[1] as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  struct addrinfo *result;
  int res = getaddrinfo(NULL, portnum, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
  printf( "getaddrinfo failed: %s", gai_strerror(res));
    return -1;
  }

  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int listen_fd = -1;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      printf("socket() failed:%s \n ", strerror(errno));
      listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      PrintOut(listen_fd, rp->ai_addr, rp->ai_addrlen);

      // Return to the caller the address family.
      *sock_family = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_fd);
    listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (listen_fd == -1)
    return listen_fd;

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_fd, SOMAXCONN) != 0) {
    printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    close(listen_fd);
    return -1;
  }

  // Return to the client the listening file descriptor.
  return listen_fd;
}

void 
HandleClient(int c_fd, struct sockaddr *addr, size_t addrlen,
                  int sock_family) {
  // Print out information about the client.
  /*
  printf("\nNew client connection \n" );
  PrintOut(c_fd, addr, addrlen);
  PrintReverseDNS(addr, addrlen);
  PrintServerSide(c_fd, sock_family);
*/
  // Loop, reading data and echo'ing it back, until the client
  // closes the connection.
  while (1) {
    char clientbuf[1024];
    ssize_t res = read(c_fd, clientbuf, 1023);
    if (res == 0) {
      printf("[The client disconnected.] \n");
      break;
    }

    if (res == -1) {
      if ((errno == EAGAIN) || (errno == EINTR))
        continue;

    printf(" Error on client socket:%s \n ", strerror(errno));
      break;
    }
    clientbuf[res] = '\0';
    printf("the client sent: %s \n", clientbuf);

    // Really should do this in a loop in case of EAGAIN, EINTR,
    // or short write, but I'm lazy.  Don't be like me. ;)
   // write(c_fd, "You typed: ", strlen("You typed: "));
    //write(c_fd, clientbuf, strlen(clientbuf));
  }

  close(c_fd);
}
























void* tree(void* info) {

//allows for passed information to be read
struct info *p = info;
  struct info thisinfo = *p;
  int fd_socket = thisinfo.fd;
  
  //openes file or creates it
  int32_t fd_file = open("database", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
  //structs and variables needed
  struct msg s;
  struct msg l;
  int res = 0;
  

//main loop
while(1){
     //read the struct from the client
    read(fd_socket, &s, 516);
    //decided task that needs to be compleated

    //put task
if (s.type==1){
res = adds(fd_file,s);

 //if succsesful then it says it back
if (res==1){
  s.type=4;
 write(fd_socket, &s, 516);
  
}
}

//get task
if (s.type==2){
//get
  //send comand for get and returns it
l=get(fd_file,s.rd.id);
if(l.type==5){
memset(&l, 0, 516);

}
//sends back results
write(fd_socket, &l, 516);
}

//delete task
if (s.type==3){
  //main task
res = del(fd_file,s.rd.id);
//works
if (res==1){
  s.type=4;
 write(fd_socket, &s, 516);
}
//fails
if (res==0){
s.type=5;
 write(fd_socket, &s, 516);
}

}

//end task
if (s.type==5){
//kill
  close(fd_file);
  close(fd_socket);
  
return NULL;
}



}
return NULL;
}


int adds(int32_t fd,struct msg s)
{
//define variables
  off_t fsize;
  struct msg p;
  fsize = lseek(fd, 0, SEEK_END);
  //how many pottential structs in the file
  int turns = fsize / 512;
  
//itteate till first open file
  for (int i = 0; i < turns + 1; i++)
  {
    //makes pointer clean
    memset(&p.rd, 0, 512);
    //goes to position and makes pointer into a struct
    int pl = lseek(fd, i *512, SEEK_SET);
    int L = read(fd, &p.rd, 512);
    //detects errors
    if (pl == -1 || L == -1)
    {
      printf("error");
    }

    //detects if spot is empty
    if (p.rd.name[0] == '\0')
    {
//places struct in position
      write(fd, &s.rd, sizeof(s.rd));
      return 1;
    }
   
  }
  
return 0;
}

////////////////////////////////////////////////////////////////////
struct msg get(int32_t fd,int in)
{
 

//make variables
  off_t fsize;
  struct msg p;
  fsize = lseek(fd, 0, SEEK_END);
//how many iterations are possible
  int turns = fsize / 512;
 
//move through the file
  for (int i = 0; i < turns; i++)
  {
    //clear pointer stuct
    memset(&p.rd, 0, 512);
    //goes to place in file and then set pointer to it
    int pl = lseek(fd, i *512, SEEK_SET);
    int L = read(fd, &p.rd, 512);
    if (pl == -1 || L == -1)
    {
      printf("error");
    }

   //id pointer equals the given id the return pointer
    if (p.rd.id == in)
    {
     
      p.type=4;
      return p;
      
    }
    
  }
  //else its a fail
  p.type=5;
return p;
 

}
///////////////////////////////////////////
int del(int32_t fd,int in)
{

 //declair files
  char space[] = "\0";
  off_t fsize;
  struct msg p;
  fsize = lseek(fd, 0, SEEK_END);

  //structs in file
  int turns = fsize / 512;
  
//go through file
  for (int i = 0; i < turns; i++)
  {
    //pointer is cleared
    memset(&p.rd, 0, 512);

    //goes to position and writes to pointer
    int pl = lseek(fd, i *512, SEEK_SET);
    int L = read(fd, &p.rd, 512);
    if (pl == -1 || L == -1)
    {
      printf("error");
    }

   //if current id is same as given id
    if (p.rd.id == in)
    {
     //go through position in the file
      for (int j = 0; j < 511; j++)
      {
        //replace with null values
        lseek(fd, i *512 + j, SEEK_SET);
        write(fd, &space, 1);
      }

      return 1;
    }
 
  }
  return 0;
}