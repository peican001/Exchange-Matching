#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <fstream>
#include <sstream>
#include <string>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;

class Account{

};

void error(const char * msg);

/*****Create xml file******/
//create
int createXML(const char * xmlPath, const char * root_name);
int insertAccount(int account_id, float balance, const char * xmlPath);
XMLElement * insertSymbol(string sym_name, const char * xmlPath);
int insertAccountInSymbol(XMLElement * symNode, int account_id, int num, const char * xmlPath);


//transactions
int insertOrder(const char * xmlPath, string sym, int amount, int limit);
int insertNodeTrans(const char * xmlPath, string node_name, int trans_id);

/*****send request to server*****/
void connect_server(const char * hostname, const char * port);
void send_request(const char * file);
void receive_results();



#endif