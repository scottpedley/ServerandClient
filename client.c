#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

#define BUF 256
void put(int socket_fd);
void get(int socket_fd);
void delete(int socket_fd);
void end(int socket_fd);
void Usage(char *progname);

int LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd);

int 
main(int argc, char **argv) {

int8_t choice, flag;

  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }

  // Read something from the remote host.
  // Will only read BUF-1 characters at most.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 flag = 1;
  while(flag){

choice = 0;
//get user input
    printf("Enter your choice (1 to put, 2 to get, 3 to delete, 0 to quit): \n");
  scanf("%" SCNd8 "%*c", &choice);

//put comand 
  if (choice == 1){
  
      put(socket_fd);
    }
    //get command
    if (choice == 2){
    
      get(socket_fd);
    }
    //delete command
      if (choice == 3){
    
      delete(socket_fd);
    }
    //end command
        if (choice == 0){
    
          end(socket_fd);
      flag = 0;
    }
  

  
}
  

  
  close(socket_fd);
  return EXIT_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





void 
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int 
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int 
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }

  *ret_fd = socket_fd;
  return 1;
}

void put(int socket_fd){
//needed variables
  int res;
  struct msg s;
  s.type=PUT;

  //get used input
  printf("Enter the student name: \n"); 
  fgets(s.rd.name, MAX_NAME_LENGTH, stdin);
  s.rd.name[strlen(s.rd.name) - 1] = '\0';
  printf("Enter the student id: \n");
  scanf("%"SCNd32 "%*c", &(s.rd.id));

  //send information to server
  write(socket_fd, &s, sizeof(s));
//clear the struct
  memset(&s, 0, 516);
   //read from server
    res = read(socket_fd, &s, 516);
   if(res==-1){printf("error\n");}

   //print result
   if (s.type==4){
printf("success\n");
   }
   else{
printf("fail\n");

   }

    


}
void get(int socket_fd){
  //needed variables
  int res;
  struct msg s;
  s.type=GET;

 //get used input
 printf("Enter the student id: \n");
  scanf("%"SCNd32 "%*c", &(s.rd.id));
  
 //send information to server
  write(socket_fd, &s, sizeof(s));

  //clear the struct
  memset(&s, 0, 516);
  
  //read from server
   res = read(socket_fd, &s, 516);
  if(res==-1){printf("error\n");}
  
  //print result
    if (s.type==4){
printf("student name: %s \n", s.rd.name);
printf("student id: %d \n", s.rd.id);
printf("success\n");
   }
   else{
printf("fail\n");

   }
  


}
void delete(int socket_fd){
  //needed variables
  int res;
 struct msg s;
  s.type=DEL;

//get used input
 printf("Enter the student id: \n");
  scanf("%"SCNd32 "%*c", &(s.rd.id));
write(socket_fd, &s, sizeof(s));

//clear the struct
memset(&s, 0, 516);

 //read from server
   res = read(socket_fd, &s, 516);
   if(res==-1){printf("error\n");}
   
    //print result
    if (s.type==4){
printf("success\n");
   }
   else{
printf("fail\n");
   }


}

void end(int socket_fd){
  //ends the program
 struct msg s;
  s.type=FAIL;

write(socket_fd, &s, sizeof(s));

 

}
