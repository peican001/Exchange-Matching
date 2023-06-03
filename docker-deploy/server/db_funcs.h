#ifndef __DB_FUNCS_H__
#define __DB_FUNCS_H__

#include <iostream>
#include <string>
#include <fstream>
#include <pqxx/pqxx>
#include <ctime>

#include "assert.h"
#include "spexception.h"

#include "results.h"


#define CREATE_TABLE "createTB.txt"
#define DROP_TABLE "dropTB.txt"
#define DB_INFO "dbname=STOCK_MARKET user=postgres password=passw0rd"

using namespace std;
using namespace pqxx;

void executeQuery(string query, connection *C);

void connectDB(connection **C);

void disconnectDB(connection *C);

void ExecuteSql(string filename, connection* C);

bool checkAccount(long account, connection * C);

bool createAccount(long account, double balance, connection *C);

double getBalance(long account_id, connection *C);

void setBalance(long account_id, double remain, connection *C, int version);

bool checkSymbol(string symbol_name, connection *C);

void createSymbol(string symbol_name, connection *C);

bool createPosition(string symbol_name, long account_id, double share, connection *C);

double getShares(string &sym_name, long account_id, connection *C);

void setShares(string &sym_name, long account_id, double amount, connection *C, int version);

bool checkTransaction(long trans_id, connection *C);

void queryTrans(long trans_id, connection *C, XMLDocument * results_doc, long account_id);

long getCurrTime();

void executeOrder(long trans_id, long account_id, double amount, double dealPrice, long time, work & W);

void executePairDeal(long buyer_tran_id, long seller_tran_id, long buyer_account_id, long seller_account_id, double amount, double dealPrice, long time, work & W);

void updateOpenorder(work & W, long trans_id, double amount, int version);

//void updateTransactionStatus(work & W, long trans_id, double amount, string status);

void updateShareAndMoney(work & W,long buyer_id, long seller_id, string & sym, double shares, double balance);

//bool updateTransactionStatus(work & W, long open_trans_id, long close_trans_id, double amountRemain, long buyer_id, long seller_id, string & sym, double shares, double balance);

bool matchOrderBuyer(connection *C, long buyer_tran_id);

bool matchOrderSeller(connection *C, long seller_tran_id);

void cancelResult(connection * C, long trans_id, XMLDocument * results_doc, long account_id);

long insertTrans(work &W, long account_id, string sym, double amount, double limit_price);

void processOrder(connection *C, long account_id,string sym, double amount, double limit_price, XMLDocument * results_doc);


#endif