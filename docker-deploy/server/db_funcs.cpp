#include "db_funcs.h"

void executeQuery(string query, connection *C){
    work W(*C);
    W.exec(query);
    W.commit();
}

void connectDB(connection **C){
    try{
        *C = new connection("dbname=STOCK_MARKET user=postgres password=passw0rd host=db port=5432");
        if ((*C)->is_open()) {
            string command = "set transaction isolation level serializable;";
            executeQuery(command, *C);
            cout << "Opened database successfully: " << (*C)->dbname() << endl;
        } else {
            throw spException("Error when opening database",": (*C)->dbname()");
        }
    } catch (const std::exception &e){
        cerr << e.what() << std::endl;
        return;
    }
}

void disconnectDB(connection *C){
    if(C->is_open()){
        string dbname = C->dbname();
        C->disconnect();
        cout << "Closed database successfully: " << dbname << endl;
    }
}

void ExecuteSql(string filename, connection* C){
    string line, sql_cmd;
    std::stringstream ss;
    ifstream ifs;
    ifs.open(filename.c_str(), ifstream::in);

    if(!ifs.is_open()){
        cerr << "error: cannot open file " << filename << endl;
        exit(EXIT_FAILURE);
    }

    while (getline(ifs, line)) {
        sql_cmd += line;
        ss << line << std::endl;
        //cout << line << endl;
    }
    ifs.close();

    /* Create a transactional object. */

    executeQuery(ss.str(), C);

    /* Execute SQL query */
}

bool checkAccount(long account, connection * C){
    nontransaction N(*C);
    stringstream sqlcmd;
    sqlcmd << "SELECT * FROM ACCOUNT WHERE ACCOUNT_ID="
    << N.quote(account) << ";";
    result r(N.exec(sqlcmd.str()));
    return r.size() == 0;
}

bool createAccount(long account, double balance, connection *C){
    work W(*C);
    stringstream sqlcmd;
    sqlcmd<< "INSERT INTO ACCOUNT (ACCOUNT_ID, BALANCE) VALUES ("<<W.quote(account)<<", "<<W.quote(balance)<<");";
    try{
        W.exec(sqlcmd.str());
        W.commit();
    }catch(const exception &e){
        cerr<<e.what()<<endl;
        W.abort();
        return false;
    }
    return true;
}

double getBalance(long account_id, connection *C){
    work W(*C);
    stringstream sqlcmd;
    sqlcmd << "SELECT BALANCE FROM ACCOUNT WHERE ACCOUNT_ID = " << W.quote(account_id) << ";";
    result r(W.exec(sqlcmd.str()));
    if(r.size() == 0) return 0;
    W.commit();
    return r.begin()[0].as<double>();
    
}

void setBalance(long account_id, double remain, connection *C, int version){
    work W(*C);
    stringstream sqlcmd;
    sqlcmd << "UPDATE ACCOUNT " << "SET VERSION = ACCOUNT.VERSION+1, BALANCE = " << W.quote(remain) << "WHERE ACCOUNT_ID = " << W.quote(account_id) << " AND VERSION = " << W.quote(version) <<";";
    //W.exec(sqlcmd.str());
    result R(W.exec(sqlcmd.str()));
    result::size_type rows = R.affected_rows();
    W.commit();
    if (rows == 0) {
        cout<<"Invalid update: version of this order does not match.\n"<<endl;
        throw VersionErrorException(
            "Invalid update: version of this order does not match.\n");
    }
    
}

bool checkSymbol(string symbol_name, connection *C){
    nontransaction N(*C);
    stringstream sqlcmd;
    sqlcmd<<"SELECT * FROM SYMBOL WHERE SYMBOL_NAME="
    <<N.quote(symbol_name)<<";";
    result r(N.exec(sqlcmd.str()));
    return r.size() == 0;
}

void createSymbol(string symbol_name, connection *C){
    work W(*C);
    stringstream sqlcmd;
    sqlcmd << "INSERT INTO SYMBOL (SYMBOL_NAME) VALUES (" << W.quote(symbol_name) << ");";
    try{
        W.exec(sqlcmd.str());
        W.commit();
    }catch(const exception &e){
        cerr<<e.what()<<endl;
        W.abort();
    }
    
}

bool createPosition(string symbol_name, long account_id, double share, connection *C){
    if(checkSymbol(symbol_name, C)){
        createSymbol(symbol_name, C);
    }
    work W(*C);

    stringstream sqlcmd;
    sqlcmd<<"INSERT INTO POSITION "
    <<"(SYMBOL_NAME, ACCOUNT_ID, AMOUNT) "<<"VALUES ("
    <<W.quote(symbol_name)<<", "<<W.quote(account_id)<<", "<<W.quote(share)<<")"
    <<"ON CONFLICT ON CONSTRAINT POSITION_PK "
    <<"DO UPDATE SET AMOUNT = POSITION.AMOUNT + "
    <<W.quote(share)<<", VERSION = POSITION.VERSION + 1"<< ";";
    try{
        W.exec(sqlcmd.str());
        W.commit();
    }catch(exception & e){
        W.abort();
        cerr<<e.what()<<endl;
        return false;
    }
    return true;
}

double getShares(string &sym_name, long account_id, connection *C){
    work W(*C);
    stringstream sqlcmd;
    sqlcmd << "SELECT AMOUNT FROM POSITION WHERE ACCOUNT_ID = "
    <<W.quote(account_id)
    <<" AND SYMBOL_NAME = "<<W.quote(sym_name)<<";";
    result r(W.exec(sqlcmd.str()));
    if (r.size() == 0) return 0;
    W.commit();
    return r.begin()[0].as<double>();
}

void setShares(string &sym_name, long account_id, double amount, connection *C, int version){
    work W(*C);
    stringstream sqlcmd;
    sqlcmd << "UPDATE POSITION "
    <<"SET VERSION = POSITION.VERSION+1, AMOUNT = "<<W.quote(amount)
    <<" WHERE ACCOUNT_ID = "<<W.quote(account_id)
    <<" AND SYMBOL_NAME = "<<W.quote(sym_name) << " AND VERSION = " << W.quote(version) <<";";
    //W.exec(sqlcmd.str());
    result R(W.exec(sqlcmd.str()));
    result::size_type rows = R.affected_rows();
    W.commit();
    if (rows == 0) {
        cout<<"Invalid update: version of this order does not match.\n"<<endl;
        throw VersionErrorException(
            "Invalid update: version of this order does not match.\n");
    }
    
}

bool checkTransaction(long trans_id, connection *C){
    nontransaction N(*C);
    stringstream sqlcmd;
    sqlcmd << "SELECT TRANS_ID FROM OPENORDER WHERE TRANS_ID="
    << N.quote(trans_id) << ";";
    result r(N.exec(sqlcmd.str()));
    return r.size() == 0;
}//need recheck

void queryTrans(long trans_id, connection *C, XMLDocument * results_doc, long account_id){
    nontransaction N(*C);
    
    //bool isMatch = true;

    stringstream sqlcmd_open;
    sqlcmd_open << "SELECT AMOUNT FROM OPENORDER WHERE TRANS_ID="
    << N.quote(trans_id) << " AND ACCOUNT_ID=" << N.quote(account_id)
    <<";";
    result r_open(N.exec(sqlcmd_open.str()));
    bool isOpen = (r_open.size()==1);

    stringstream sqlcmd_canceled;
    sqlcmd_canceled << "SELECT AMOUNT, TIME FROM CANCELEDORDER WHERE TRANS_ID="
    << N.quote(trans_id) << " AND ACCOUNT_ID=" << N.quote(account_id)<< ";";
    result r_canceled(N.exec(sqlcmd_canceled.str()));
    bool isCanceled = (r_canceled.size()==1);
    
    stringstream sqlcmd_executed;
    sqlcmd_executed << "SELECT DEAL_PRICE, AMOUNT, TIME FROM EXECUTEDORDER WHERE TRANS_ID="
    << N.quote(trans_id) << " AND ACCOUNT_ID=" << N.quote(account_id)<< ";";
    result r_executed(N.exec(sqlcmd_executed.str()));
    vector<double> executed_shares, executed_price;
    vector<long> executed_time;
    for(result::iterator i_executed = r_executed.begin();i_executed!=r_executed.end();i_executed++){
        executed_price.push_back(i_executed[0].as<double>());
        executed_shares.push_back(i_executed[1].as<double>());
        executed_time.push_back(i_executed[2].as<long>());
    }

    result::iterator it;
    double share;
    long cancel_time;

    if (!isOpen && !isCanceled && executed_shares.size() == 0){
        resultsErrorNode(results_doc, trans_id, "query", "trans id does not exist or account id and trans id do not match!");
        return;
    }

    XMLElement * status_node = resultsAddStatus(results_doc, trans_id);

    if (isOpen){
        it = r_open.begin();
        share = it[0].as<double>();
        resultsStatusOpen(results_doc, status_node, share);
    }
    else if(isCanceled){
        it = r_canceled.begin();
        share = it[0].as<double>();
        cancel_time = it[1].as<long>();
        resultsStatusCanceled(results_doc, status_node, share, cancel_time);
    }

    if(executed_shares.size() > 0){
        for (int i = 0; i < executed_shares.size(); i++){
            resultsStatusExecuted(results_doc, status_node, executed_shares[i], executed_price[i], executed_time[i]);
        }
    }

    return;

    /*
    Result res;
    if(status=="OPEN"){
        return res.queryResult(trans_id,trans_share,deal_shares,deal_price,deal_time);
    }else if(status=="CANCELED"){
        stringstream sqlcmd_cancel;
        sqlcmd_cancel << "SELECT CANCEL_TIME FROM CANCELTIME WHERE TRANS_ID="
        << N.quote(trans_id) << ";";
        result rC(N.exec(sqlcmd_cancel.str()));
        long cancel_time = rC.begin()[0].as<long>();
        return res.queryResult(trans_id,trans_share,cancel_time,deal_shares,deal_price,deal_time);
    }else{
        cerr<<"A transaction with neither open nor canceled status: "<<status<<endl;
        return "";
    }
    */
}

long getCurrTime(){
    //return the seconds from 1970 00:00:00
    time_t now_time = time(nullptr);
    stringstream ss;
    ss << now_time;
    long time;
    ss >> time;
    return time;
}

void executeOrder(long trans_id, long account_id, double amount, double dealPrice, long time, work & W){

    stringstream sqlcmd;
    sqlcmd << "INSERT INTO EXECUTEDORDER "
    <<"(TRANS_ID, ACCOUNT_ID, AMOUNT, DEAL_PRICE, TIME) "
    <<"VALUES ("<<W.quote(trans_id)<<", "<<W.quote(account_id)<<", "
    <<W.quote(amount)<<", "<<W.quote(dealPrice)<<", "
    <<W.quote(time)<<");";

    W.exec(sqlcmd.str());

}

void executePairDeal(long buyer_tran_id, long seller_tran_id, long buyer_account_id, long seller_account_id, double amount, double dealPrice, long time, work & W){
    executeOrder(buyer_tran_id, buyer_account_id, amount, dealPrice, time, W);
    executeOrder(seller_tran_id, seller_account_id, -1 * amount, dealPrice, time, W);//wait for revise
}

void updateOpenorder(work & W,long trans_id, double amount, int version){

    stringstream ss;
    if(amount == 0){
        ss<<"DELETE FROM OPENORDER WHERE TRANS_ID="
        <<W.quote(trans_id) << " AND VERSION = " << to_string(version) << ";";
    }
    else{
        ss<<"UPDATE OPENORDER ";
        ss<<"SET VERSION = " << to_string(version + 1);
        ss<<", AMOUNT="<<amount;
        ss<<" Where TRANS_ID="<<trans_id<< " AND VERSION = " << to_string(version) << ";";
    }
    //W.exec(ss.str());
    result Updates(W.exec(ss.str()));
    result::size_type rows = Updates.affected_rows();
    if (rows == 0) {
        throw VersionErrorException(
            "Invalid update: version of this order does not match.\n");
    }

}


// Buyer got Shares and Seller got balance.
void updateShareAndMoney(work & W,long buyer_id, long seller_id, string & sym, double shares, double balance){

    stringstream ss;
    // Update Position
    // ss<<"UPDATE POSITION ";
    // ss<<"SET AMOUNT=POSITION.AMOUNT+"<<shares;
    // ss<<" Where POSITION.SYMBOL_NAME="<<W.quote(sym)<<" And POSITION.ACCOUNT_ID="<<buyer_id<<";";
    ss<<"INSERT INTO POSITION "
    <<"(SYMBOL_NAME, ACCOUNT_ID, AMOUNT) "<<"VALUES ("
    <<W.quote(sym)<<", "<<W.quote(buyer_id)<<", "<<W.quote(shares)<<")"
    <<"ON CONFLICT ON CONSTRAINT POSITION_PK "
    <<"DO UPDATE SET AMOUNT = POSITION.AMOUNT + "
    <<W.quote(shares)<<", VERSION = POSITION.VERSION + 1"<< ";";
    
    // Update Balance

    ss<<"UPDATE ACCOUNT ";
    ss<<"SET VERSION = ACCOUNT.VERSION + 1, BALANCE=ACCOUNT.BALANCE+"<<balance;
    ss<<" Where ACCOUNT.ACCOUNT_ID="<<seller_id<<";";

    W.exec(ss.str());

}

/*
void updateTransactionStatus(work & W, long trans_id, double amount, string status){
    stringstream ss;
    ss<<"UPDATE OPENORDER ";
    ss<<"SET STATUS="<<W.quote(status)<<", AMOUNT="<<amount;
    ss<<" Where OPENORDER.TRANS_ID="<<trans_id<<";";
    W.exec(ss.str());
}

bool updateTransactionStatus(work & W, long open_trans_id, long close_trans_id, double amountRemain, long buyer_id, long seller_id, string & sym, double shares, double balance){
    try{
        updateTransactionStatus(W, open_trans_id, amountRemain, "OPEN");
        updateTransactionStatus(W, close_trans_id, 0, "OPEN");
        updateShareAndMoney(W, buyer_id, seller_id, sym, shares, balance);
        W.commit();
        return true;
    }catch(exception & e){
        cerr<<e.what()<<endl;
        W.abort();
        return false;
    }
    
}
*/

void updateTransactionStatus(work & W, long open_trans_id, long close_trans_id, double amountRemain, long buyer_id, long seller_id, string & sym, double shares, double balance){
    try{
        //updateOpenorder(W, open_trans_id, amountRemain);
        //updateOpenorder(W, close_trans_id, 0);
        updateShareAndMoney(W, buyer_id, seller_id, sym, shares, balance);
        W.commit();
        return;
    }catch(exception & e){
        cerr<<e.what()<<endl;
        W.abort();
        return;
    }
    
}

/*
    The function will return true if we need further match. 
    will return false if we do not need to match.
*/
bool matchOrderBuyer(connection *C, long buyer_tran_id){  //, string & sym, double amount, double price, long buyer_id
    // This is for buyer
    // Select eligible Transaction ID, price, amount, from TRANSACTIONS 
    // Where TRANSACTIONS.status = "OPEN" and TRANSACTIONS.SYMBOL_NAME="sym" and TRANSACTIONS.LIMIT_PRICE < 0 and TRANSACTIONS.LIMIT_PRICE > (-price) 
    // Order By price ASC
    work W(*C);
    stringstream ss;
    cout<<ss.str()<<endl;
    // Get the Buyer Transaction Information. 
    ss<<"SELECT TRANS_ID, SYMBOL_NAME, ACCOUNT_ID, LIMIT_PRICE, AMOUNT, TIME, VERSION";
    ss<<" FROM OPENORDER";
    ss<<" WHERE TRANS_ID="<<buyer_tran_id<<";";
    result buyer(W.exec(ss.str()));
    auto buyerInfo = buyer.begin();
    string sym = buyerInfo[1].as<string>();
    long buyer_id = buyerInfo[2].as<long>();
    double price = buyerInfo[3].as<double>(), amount = buyerInfo[4].as<double>();
    int buyer_version = buyerInfo[6].as<int>();
    ss.str("");
    ss<<"SELECT TRANS_ID, SYMBOL_NAME, ACCOUNT_ID, LIMIT_PRICE, AMOUNT, TIME, VERSION";
    ss<<" FROM OPENORDER ";
    ss<<"WHERE SYMBOL_NAME="<<W.quote(sym);
    ss<<" AND AMOUNT < 0 AND LIMIT_PRICE <= "<<(price);
    ss<<" ORDER BY LIMIT_PRICE ASC, TIME ASC;";
    result r(W.exec(ss.str()));
    auto c = r.begin();
    if(c == r.end()){
        W.commit();
        return false;
    }
    long seller_id=c[2].as<long>(), seller_tran_id = c[0].as<long>();
    double buyerAmountRemain=amount+c[4].as<double>(), shares=-c[4].as<double>();
    double dealPrice = buyerInfo[5].as<long>() <= c[5].as<long>() ? price : c[3].as<double>();
    int seller_version = c[6].as<int>();
    long time = getCurrTime();
    if(buyerAmountRemain <= 0){
        //seller sell part of shares
        double sellerAmountRemain = buyerAmountRemain;
        double totalPrice = amount*dealPrice;
        updateOpenorder(W, seller_tran_id, sellerAmountRemain, seller_version);
        updateOpenorder(W, buyer_tran_id, 0, buyer_version);

        executePairDeal(buyer_tran_id,seller_tran_id,buyer_id, seller_id, amount,dealPrice,time,W);
        /*
        bool success = updateTransactionStatus(W,seller_tran_id,buyer_tran_id,sellerAmountRemain,buyer_id,seller_id,sym,amount,totalPrice);
        if(!success){
            return matchOrderBuyer(C,buyer_tran_id);
        }
        //return false to continue trying to match more trans
        return false;
        */

       stringstream sql_refund;
        sql_refund<<"UPDATE ACCOUNT "
        <<"SET BALANCE=ACCOUNT.BALANCE+"<<((price-dealPrice)*amount)<<", VERSION = ACCOUNT.VERSION+1"
        <<" Where ACCOUNT.ACCOUNT_ID="<<buyer_id<<";";
        W.exec(sql_refund.str());
       updateTransactionStatus(W,seller_tran_id,buyer_tran_id,sellerAmountRemain,buyer_id,seller_id,sym,amount,totalPrice);

        
        //W.commit();

       return false;
    }else{
        // Seller sells all of his shares.
        double totalPrice = shares*dealPrice;
        updateOpenorder(W, buyer_tran_id, buyerAmountRemain, buyer_version);
        updateOpenorder(W, seller_tran_id, 0, seller_version);

        executePairDeal(buyer_tran_id,seller_tran_id,buyer_id, seller_id, shares,dealPrice,time,W);
        /*
        bool success = updateTransactionStatus(W,buyer_tran_id,seller_tran_id,buyerAmountRemain,buyer_id,seller_id,sym,shares,totalPrice);
        if(!success){
            return matchOrderBuyer(C,buyer_tran_id);
        }
        return true;
        // Return true is the buyer has done his transaction
        */
       stringstream sql_refund;
        sql_refund<<"UPDATE ACCOUNT "
        <<"SET BALANCE=ACCOUNT.BALANCE+"<<((price-dealPrice)*shares)<<", VERSION = ACCOUNT.VERSION+1"
        <<" Where ACCOUNT.ACCOUNT_ID="<<buyer_id<<";";
        W.exec(sql_refund.str());
       updateTransactionStatus(W,buyer_tran_id,seller_tran_id,buyerAmountRemain,buyer_id,seller_id,sym,shares,totalPrice);
       
        //W.commit();
       return true;
    }
}

/*
    The function will return true if we need further match. 
    will return false if we do not need to match.
*/
bool matchOrderSeller(connection *C, long seller_tran_id){//, string & sym, double amount, double price, long seller_id
    // This is for seller
    // Select eligible Transaction ID, price, amount, from TRANSACTIONS 
    // Where TRANSACTIONS.status = "OPEN" and TRANSACTIONS.SYMBOL_NAME="sym" and TRANSACTIONS.LIMIT_PRICE < 0 and TRANSACTIONS.LIMIT_PRICE > (-price) 
    // Order By price ASC
    work W(*C);
    stringstream ss;

    ss<<"SELECT TRANS_ID, SYMBOL_NAME, ACCOUNT_ID, LIMIT_PRICE, AMOUNT, TIME, VERSION";
    ss<<" From OPENORDER";
    ss<<" Where OPENORDER.TRANS_ID="<<seller_tran_id<<";";
    result seller(W.exec(ss.str()));
    auto sellerInfo = seller.begin();
    string sym = sellerInfo[1].as<string>();
    long seller_id = sellerInfo[2].as<long>();
    double price = sellerInfo[3].as<double>(), amount = sellerInfo[4].as<double>();
    int seller_version = sellerInfo[6].as<int>();
    ss.str("");

    ss<<"SELECT TRANS_ID, SYMBOL_NAME, ACCOUNT_ID, LIMIT_PRICE, AMOUNT, TIME, VERSION";
    ss<<" From OPENORDER";
    ss<<" Where OPENORDER.SYMBOL_NAME="<<W.quote(sym);
    ss<<" and OPENORDER.AMOUNT > 0 and OPENORDER.LIMIT_PRICE >= "<<(price);
    ss<<" Order By LIMIT_PRICE DESC, TIME ASC;";
    result r(W.exec(ss.str()));
    auto c = r.begin();
    if(c == r.end()){
        W.commit();
        return false;
    }
    long buyer_id=c[2].as<long>(), buyer_tran_id = c[0].as<long>();
    double sellerAmountRemain=amount+c[4].as<double>(), shares=c[4].as<double>();
    double dealPrice = sellerInfo[5].as<long>() <= c[5].as<long>() ? price : c[3].as<double>();
    double buyer_price = c[3].as<double>();
    int buyer_version = c[6].as<int>();
    long time = getCurrTime();
    if(sellerAmountRemain < 0){
        //seller sell part of shares
        //buyerremain = 0 sellerremain<0,
        double totalPrice = shares*dealPrice;
        updateOpenorder(W, seller_tran_id, sellerAmountRemain, seller_version);
        updateOpenorder(W, buyer_tran_id, 0, buyer_version);
        executePairDeal(buyer_tran_id,seller_tran_id,buyer_id, seller_id, shares,dealPrice,time,W);
        /*
        bool success = updateTransactionStatus(W,seller_tran_id,buyer_tran_id,sellerAmountRemain,seller_id,seller_id,sym,shares,totalPrice);
        if(!success){
            return matchOrderSeller(C,seller_tran_id);
        }
        //return false to continue trying to match more trans
        return true;
        */

       stringstream sql_refund;
        sql_refund<<"UPDATE ACCOUNT "
        <<"SET BALANCE=ACCOUNT.BALANCE+"<<((buyer_price-dealPrice)*shares)<< ", VERSION = ACCOUNT.VERSION+1"
        <<" Where ACCOUNT.ACCOUNT_ID="<<buyer_id<<";";
        W.exec(sql_refund.str());
       updateTransactionStatus(W,seller_tran_id,buyer_tran_id,sellerAmountRemain,seller_id,seller_id,sym,shares,totalPrice);

       
        //W.commit();
       return true;
    }else{
        // seller sells all shares.
        double buyerAmountRemain = sellerAmountRemain;
        double totalPrice = -amount*dealPrice;
        updateOpenorder(W, buyer_tran_id, buyerAmountRemain, buyer_version);
        updateOpenorder(W, seller_tran_id, 0, seller_version);
        executePairDeal(buyer_tran_id,seller_tran_id,buyer_id, seller_id, -amount,dealPrice,time,W);
        /*
        bool success = updateTransactionStatus(W,buyer_tran_id,seller_tran_id,buyerAmountRemain,seller_id,seller_id,sym,amount,totalPrice);
        if(!success){
            return matchOrderSeller(C,seller_tran_id);
        }
        return false;
        // Return true is the seller has done his transaction
        */
    stringstream sql_refund;
        sql_refund<<"UPDATE ACCOUNT "
        <<"SET BALANCE=ACCOUNT.BALANCE+"<<((buyer_price-dealPrice)*(-1)*amount)<< ", VERSION = ACCOUNT.VERSION+1"
        <<" Where ACCOUNT.ACCOUNT_ID="<<buyer_id<<";";
        W.exec(sql_refund.str());

       updateTransactionStatus(W,buyer_tran_id,seller_tran_id,buyerAmountRemain,seller_id,seller_id,sym,-amount,totalPrice);
        
        //W.commit();
       return false;
    }
}

void cancelResult(connection * C, long trans_id, XMLDocument * results_doc, long account_id){

    work W(*C);
    
    stringstream sqlcmd_tran;
    sqlcmd_tran << "SELECT AMOUNT, LIMIT_PRICE, SYMBOL_NAME FROM OPENORDER WHERE TRANS_ID="
    << W.quote(trans_id) << " AND ACCOUNT_ID=" << W.quote(account_id)<<";";
    result rT(W.exec(sqlcmd_tran.str()));

    bool isOpen = (rT.size() == 1);

    if (rT.size() == 0){
        resultsErrorNode(results_doc, trans_id, "cancel", "trans id has no open order or account id and trans id do not match!");
        return;
    }
    else if(rT.size() > 1){
        cerr << "openorder has mutiple rows with same trans ID!" << endl;
        exit(EXIT_FAILURE);
    }

    

    auto rti = rT.begin();
    double canceledShare = rti[0].as<double>(); 
    double canceledPrice = rti[1].as<double>(); 
    string sym = rti[2].as<string>();

    stringstream sqlUpdate;
    if (canceledShare < 0){
        // sqlUpdate<<"UPDATE POSITION "
        // <<"SET AMOUNT=POSITION.AMOUNT+"<<((-1)*canceledShare)
        // <<" Where POSITION.SYMBOL_NAME="<<W.quote(sym)<<" And POSITION.ACCOUNT_ID="<<account_id<<";";
        sqlUpdate<<"INSERT INTO POSITION "
        <<"(SYMBOL_NAME, ACCOUNT_ID, AMOUNT) "<<"VALUES ("
        <<W.quote(sym)<<", "<<W.quote(account_id)<<", "<<W.quote((-1)*canceledShare)<<")"
        <<"ON CONFLICT ON CONSTRAINT POSITION_PK "
        <<"DO UPDATE SET AMOUNT = POSITION.AMOUNT + "
        <<W.quote((-1)*canceledShare)<<", VERSION = POSITION.VERSION + 1"<< ";";

    }
    else if(canceledShare > 0){
        sqlUpdate<<"UPDATE ACCOUNT "
        <<"SET VERSION = ACCOUNT.VERSION+1, BALANCE=ACCOUNT.BALANCE+"<<(canceledPrice*canceledShare)
        <<" Where ACCOUNT.ACCOUNT_ID="<<account_id<<";";
    }
    else{
        cerr << "trans ID has 0 share!" << endl;
        exit(EXIT_FAILURE);
    }

    string sqlUpdateStr = sqlUpdate.str();


    stringstream sqlcmd_executed;
    sqlcmd_executed << "SELECT DEAL_PRICE, AMOUNT, TIME FROM EXECUTEDORDER WHERE TRANS_ID="
    << W.quote(trans_id) << ";";
    result r_executed(W.exec(sqlcmd_executed.str()));
    vector<double> executed_shares, executed_price;
    vector<long> executed_time;
    for(result::iterator i_executed = r_executed.begin();i_executed!=r_executed.end();i_executed++){
        executed_price.push_back(i_executed[0].as<double>());
        executed_shares.push_back(i_executed[1].as<double>());
        executed_time.push_back(i_executed[2].as<long>());
    }

    //string status = rti[0].as<string>();

    /*
    stringstream sqlcmd_deal;
    sqlcmd_deal << "SELECT PRICE, AMOUNT, TIME FROM DEAL WHERE TRANS_ID="
    << W.quote(trans_id) << ";";
    result rD(W.exec(sqlcmd_deal.str()));
    vector<double> deal_shares, deal_price;
    vector<long> deal_time;
    for(result::iterator iD = rD.begin();iD!=rD.end();iD++){
        deal_price.push_back(iD[0].as<double>());
        deal_shares.push_back(iD[1].as<double>());
        deal_time.push_back(iD[2].as<long>());
    }
    */

    stringstream sqlDelete;
    sqlDelete<<"DELETE FROM OPENORDER WHERE TRANS_ID="
        <<W.quote(trans_id);
    string sqlDeleteStr = sqlDelete.str();
    long currentTime = getCurrTime();
    /*
    Result res;
    if(status=="CANCELED"){
        return res.cancelResult(trans_id,canceledShare, currentTime,deal_shares, deal_price, deal_time); 
    }
    */
    
    stringstream sqlInsert;
    sqlInsert<<"INSERT INTO CANCELEDORDER VALUES("<<trans_id<<", "<<account_id << ", " <<canceledShare << ", " << currentTime<<");";
    string sqlInsertStr = sqlInsert.str();
    
    try{
        W.exec(sqlUpdateStr);
        W.exec(sqlDeleteStr);
        W.exec(sqlInsertStr);
        XMLElement * canceled_node = resultsAddCanceled(results_doc, trans_id);
        resultsStatusCanceled(results_doc, canceled_node, canceledShare, currentTime);
        if(executed_shares.size() > 0){
            for (int i = 0; i < executed_shares.size(); i++){
                resultsStatusExecuted(results_doc, canceled_node, executed_shares[i], executed_price[i], executed_time[i]);
            }
        }
        W.commit();
        return;
        
        //return res.cancelResult(trans_id,canceledShare, currentTime,deal_shares, deal_price, deal_time); 
    }catch(exception & e){
        //JKLDFJF
        //cerr<<"Need to Execuate again ID:"<<trans_id<<endl;
        resultsErrorNode(results_doc, trans_id, "cancel", "command failed when try to cancel!");
        W.abort();
    }
    
}


long insertTrans(work &W, long account_id, string sym, double amount, double limit_price){
    stringstream sqlcmd;
    sqlcmd<<"INSERT INTO OPENORDER "
    <<"(SYMBOL_NAME, ACCOUNT_ID, AMOUNT, LIMIT_PRICE, TIME)"
    <<"VALUES ("<<W.quote(sym)<<", "
    <<W.quote(account_id)<<", "<<W.quote(amount)<<", "
    <<W.quote(limit_price)<<", "<<W.quote(getCurrTime())<<") "
    <<"RETURNING TRANS_ID;";
    cout<<"insert amount:"<<amount<<endl;
    result r(W.exec(sqlcmd.str()));
    long trans_id = r.begin()[0].as<long>();
    return trans_id;
}

void processOrder(connection *C, long account_id,string sym, double amount, double limit_price, XMLDocument * results_doc){

    //work W(*C);
    if(amount>0){
        while(1){
            try{
                double balance = getBalance(account_id,C);
                if(balance<amount*limit_price){
                    string msg = "Insufficient balance";
                    resultsErrorOrder(results_doc, sym, amount, limit_price, msg);
                    return;
                    //return err.orderErrorMSG(sym,amount,limit_price,msg);
                }else{
                    work W(*C);
                    stringstream checksym;
                    checksym << "SELECT SYMBOL_NAME FROM SYMBOL WHERE SYMBOL_NAME=" << W.quote(sym) << ";";
                    result r1(W.exec(checksym.str()));
                    if (r1.size() == 0){
                        W.abort();
                        resultsErrorOrder(results_doc, sym, amount, limit_price, "Symbol does not exist!");
                        return;
                    }
                    
                    stringstream getversion;
                    getversion << "SELECT VERSION FROM ACCOUNT WHERE ACCOUNT_ID=" << W.quote(account_id) << ";";
                    result r(W.exec(getversion.str()));
                    W.commit();
                    int version = r[0][0].as<int>();

                    double afterBalance = balance-amount*limit_price;
                    setBalance(account_id,afterBalance,C, version);
                    //cout << "afterbalance: " << afterBalance << endl;
                    break;
                }
            }
            catch (const VersionErrorException & e) {
                continue;
            }
        }
    }else if(amount < 0){
        while(1){
            try{
                double shares = getShares(sym,account_id,C);
                if(shares<(-1)*amount){
                    string msg = "Insufficient shares";
                    resultsErrorOrder(results_doc, sym, amount, limit_price, msg);
                    return;
                    //return err.orderErrorMSG(sym,amount,limit_price,msg);
                }else{
                    work W(*C);
                    stringstream getversion;
                    getversion << "SELECT VERSION FROM POSITION WHERE ACCOUNT_ID=" << W.quote(account_id) << " AND SYMBOL_NAME = " << W.quote(sym) << ";";
                    result r(W.exec(getversion.str()));
                    W.commit();
                    int version = r[0][0].as<int>();
                    double afterShares = shares+amount;
                    setShares(sym,account_id,afterShares,C, version);
                    //cout << "aftershares: " << afterShares << endl;
                    break;
                }
            }
            catch (const VersionErrorException & e) {
                continue;
            }
        }
    }
    else{
        //deal with amount == 0
        resultsErrorOrder(results_doc, sym, amount, limit_price, "Order share amount cannot be 0!");
        return;
    }
    work W(*C);
    long trans_id = insertTrans(W,account_id,sym,amount,limit_price);
    try{
        W.commit();
        resultsOpenedOrder(results_doc, sym, amount, limit_price, trans_id);
    }catch(const exception &e){
        W.abort();
        cerr<<e.what()<<endl;
        resultsErrorOrder(results_doc, sym, amount, limit_price, "Cannot create order!");
        return;
        // cerr<<1<<endl;
        //return processOrder(C,account_id,sym,amount,limit_price);
    }
    bool keepMatching = true;
    //int i = 2;
    while(1){
        try{
            if(amount>0){
                while(keepMatching){
                    keepMatching = matchOrderBuyer(C,trans_id);
                    //cout<<"!"<<endl;
                    //i--;
                }
                break;
            }else{
                while(keepMatching){
                    keepMatching = matchOrderSeller(C,trans_id);
                    //cout<<"?"<<endl;
                    //i--;
                }
                break;
            }
        }
        catch (const VersionErrorException & e) {
                continue;
        }

    }
}