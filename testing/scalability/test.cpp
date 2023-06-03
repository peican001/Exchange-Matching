// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <iostream>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <fstream>
// #include <sstream>
// #include <string>



// using namespace std;
// int server_fd;

// /*Send request to server*/
// void error(const char * msg){
//     cerr << msg << endl;
//     exit(EXIT_FAILURE);
// }

// void connect_server(const char * hostname, const char * port){
//     int status;
//     struct addrinfo host_info;
//     struct addrinfo *host_info_list, *p;

//     memset(&host_info, 0, sizeof(host_info));
//     host_info.ai_family   = AF_UNSPEC;
//     host_info.ai_socktype = SOCK_STREAM;

//     //get ringmaster addrinfo
//     status = getaddrinfo(hostname, port, &host_info, &host_info_list);
//     if (status != 0) {
//         error("cannot get address info for host");
//     }
//     //create ringmaster socket
//     server_fd = socket(host_info_list->ai_family, 
// 		     host_info_list->ai_socktype, 
// 		     host_info_list->ai_protocol);
//     if (server_fd == -1) {
//         error("player cannot create client socket");
//     }
//     //connect to ringmaster (ringmaster_fd must be listeing ... for layer to connect)
//     status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
//     if (status == -1) {
//         error("player cannot connect to server socket");
//     }
//     freeaddrinfo(host_info_list);
//     // cout << "Connected to " << hostname << " on port " << port << endl;
// }

// void send_request(const char * file) {
//     string request_file(file);
//     ifstream fs(request_file);
//     if (fs.fail()) {
//         error("cannot open request file");
//     }
//     stringstream ss;
//     ss << fs.rdbuf();
//     string request = ss.str();
//     unsigned len = request.length();
//     request = to_string(len) + "\n" + request; //add the length of xml to the first line

//     // const char *message = "test";
//     send(server_fd, request.c_str(), request.length(), 0);
// }

// void receive_results() {
//     char buffer[65536];
//     int len = recv(server_fd, buffer, sizeof(buffer), 0);
//     // cout << "Received: \n" << buffer << endl;
// }

// int main(int argc, char *argv[]) {
//     // if (argc != 4) {
//     //     error("input form should be <hostname> <port_num> <test_file>");
//     // }
//     if (argc != 2) {
//         error("input form should be <test_file>");
//     }
//     const char * xmlPath = argv[1];

//     /*creating xml file stored in argv[1]*/
//     //-------Create-------
//     // createXML(xmlPath, "create", 0);
//     // insertAccount(123456, 1000, xmlPath);
//     // XMLElement *  SPY = insertSymbol("SPY", xmlPath);
//     // insertAccountInSymbol(SPY, 123456, 100000, xmlPath);
//     // insertAccount(9999, 988.9, xmlPath);
    
//     //-------Transactions------- 
//     // createXML(xmlPath, "transactions", 2345);
//     // insertOrder(xmlPath, "SPY", 300, 125);
//     // insertOrder(xmlPath, "SPY", -100, 130);
//     // insertOrder(xmlPath, "SPY", 200, 127);
//     // insertOrder(xmlPath, "SPY", -500, 128);
//     // insertOrder(xmlPath, "SPY", -200, 140);
//     // insertOrder(xmlPath, "SPY", 400, 125);
//     // insertNodeTrans(xmlPath, "query", 33333);
//     // insertNodeTrans(xmlPath, "cancel", 33333);

//     /*sending request of file argv[1]*/
//     // connect_server(argv[1], argv[2]);//for real program
//     //  connect_server("wille-virtual-machine", "12345");
//     connect_server("vcm-32284.vm.duke.edu", "12345");//for testing
//     send_request(xmlPath);
//     receive_results();
//     return 0;
// }


#include <arpa/inet.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <time.h>
#include <unistd.h>


#define hostname "vcm-32284.vm.duke.edu"
#define port "12345"

// change MAX_THREAD to increase or decrease the number of threads
#define MAX_THREAD 100
#define BUFF_SIZE 65535

using namespace std;

void error(const char * msg){
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

// set up the client and start sending message to server and wait to receive response
void *handler(void *arg) {
  int status, server_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list, *p;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  //get ringmaster addrinfo
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
      error("cannot get address info for host");
  }
  //create ringmaster socket
  server_fd = socket(host_info_list->ai_family, 
        host_info_list->ai_socktype, 
        host_info_list->ai_protocol);
  if (server_fd == -1) {
      error("client cannot create client socket");
  }
  int yes = 1;
  status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes,
                    sizeof(yes));

  status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
      error("client cannot connect to server socket");
  }
  freeaddrinfo(host_info_list);

  string request_file((const char *)arg);
  ifstream fs(request_file);
  if (fs.fail()) {
      error("cannot open request file");
  }
  stringstream ss;
  ss << fs.rdbuf();
  string request = ss.str();
  unsigned len = request.length();
  request = to_string(len) + "\n" + request; //add the length of xml to the first line

  // const char *message = "test";
  send(server_fd, request.c_str(), request.length(), 0);
  char buffer[65536];
  int recv_len = recv(server_fd, buffer, sizeof(buffer), 0);
  return NULL;
}

int main(int argc, char **argv) {
  int threads[MAX_THREAD];
  pthread_attr_t thread_attr[MAX_THREAD];
  pthread_t thread_ids[MAX_THREAD];

  for (int i = 0; i < MAX_THREAD; ++i) {
    threads[i] = pthread_create(&thread_ids[i], NULL, handler, argv[1]);
    usleep(1000);
  }
  for (int i = 0; i < MAX_THREAD; ++i) {
    pthread_join(thread_ids[i], NULL);
  }
  return 0;
}
