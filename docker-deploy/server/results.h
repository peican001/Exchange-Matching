#ifndef __RESULTS_H__
#define __RESULTS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;

/****Create****/
void resultsSuccessCreateAccount(XMLDocument * doc, long account_id);
void resultsSuccessCreateSymbol(XMLDocument * doc, long account_id, string sym);
void resultsErrorCreateAccount(XMLDocument * doc, long account_id);
void resultsErrorCreateSymbol(XMLDocument * doc, long account_id, string sym);


/*****Transactions*****/
//Order
void resultsErrorOrder(XMLDocument * doc, string sym, double amount, double limit, string error_msg);
void resultsOpenedOrder(XMLDocument * doc, string sym, double amount, double limit, long trans_id);

//Query & Cancel
XMLElement * resultsAddStatus(XMLDocument * doc, long trans_id);
void resultsStatusOpen(XMLDocument * doc, XMLElement * node, double shares);
void resultsStatusCanceled(XMLDocument * doc, XMLElement * node, double shares, long time);
void resultsStatusExecuted(XMLDocument * doc, XMLElement * node, double shares, double price, long time);
void resultsErrorNode(XMLDocument * doc, long trans_id, string node_name, string error_msg);

XMLElement * resultsAddCanceled(XMLDocument * doc, long trans_id);

#endif