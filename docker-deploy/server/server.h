#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <thread>
#include "tinyxml2.h"
#include "db_funcs.h"
#include "results.h"
#include <pthread.h>



#define BACKLOG 10000000
using namespace tinyxml2;
using namespace std;


void error(const char * msg);
void run();

class Client {
public:
    int client_fd;
    Client(int fd):client_fd(fd){}
};


class Server{
public:
    int server_fd, server_client_fd;
    const char * hostname = NULL;
    const char * port_num = "12345";
    connection * c;
    // pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    Server(){};
    void create_server();
    void run();
    static void * handleRequest(void * client_fd);
    static string handleChunkMsg(string & msg, int received_len, Client * client_fd);

    static void handleCreate(XMLElement * root, connection * C, Client * client_class);
    static void handleCreateAccount(XMLElement * account_node, XMLDocument * results_doc, connection * C);
    static void handleCreateSymbol(XMLElement * symbol_node, XMLDocument * results_doc, connection * C);

    static void handleTransactions(XMLElement * root, connection * C, Client * client_class);
    static void handleOrder(XMLElement * order_node, XMLDocument * results_doc, long account_id, connection * C);
    static void handleQuery(XMLElement * query_node, XMLDocument * results_doc, long account_id, connection * C);
    static void handleCancel(XMLElement * cancel_node, XMLDocument * results_doc, long account_id, connection * C);

    /*Results*/
    static void createResultsXML(XMLDocument * doc);
    // void resultsSuccessCreateAccount(XMLDocument * doc, int account_id);
    // void resultsSuccessCreateSymbol(XMLDocument * doc, int account_id, string sym);
    // void resultsErrorCreateAccount(XMLDocument * doc, int account_id);
    // void resultsErrorCreateSymbol(XMLDocument * doc, int account_id, string sym);
    static void sendResultsToClient(XMLDocument * doc, Client * client_class);


    //void resultsErrorNode(XMLDocument * doc, int trans_id, string node_name, string error_msg);

};

// void resultsErrorOrder(XMLDocument * doc, string sym, int amount, int limit, string error_msg);
// void resultsOpenedOrder(XMLDocument * doc, string sym, int amount, int limit, int trans_id);



#endif