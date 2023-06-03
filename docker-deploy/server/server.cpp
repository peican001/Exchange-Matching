#include "server.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

const int MAX_THREADS = 100000;
pthread_t threads[MAX_THREADS];
bool threads_idle[MAX_THREADS] = {true};

void error(const char * msg) {
    cerr << msg << endl;
    // exit(EXIT_FAILURE);
}

void Server::create_server() {
    int status;
    struct addrinfo host_info;
    struct addrinfo * host_info_list;
    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC; 
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    if (getaddrinfo(hostname, port_num, &host_info, &host_info_list) != 0) {
        error("getaddrinfo error!");
    }
    server_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (server_fd == -1) {
        error("server socket creation error!");
    }
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (bind(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen) != 0) {
        error("Server bind error!");
    }
    status = listen(server_fd, BACKLOG);
    if (status == -1) {
        error("Server listen error!");
    }
    freeaddrinfo(host_info_list);
    cout << "Waiting for connection on port 12345" << endl;
}

void Server::run() {
    while (true) {
        struct sockaddr_storage client_addr;
        socklen_t socket_addr_len = sizeof(client_addr);
        pthread_mutex_lock(&mutex);
        server_client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &socket_addr_len);
        cout << "Accept client" << endl;
        if (server_client_fd == -1) {
            error("accept client failed");
        }
        Client * FD = new Client(server_client_fd);
        pthread_mutex_unlock(&mutex);
        int thread_id = -1;
        for (int i = 0; i < MAX_THREADS; i++) {
            if (threads_idle[i]) {
                thread_id = i;
                threads_idle[i] = false;
                break;
            }
        }
        
        if (thread_id == -1) {
            // All threads are busy, wait for one of them to become idle
            while (true) {
                for (int i = 0; i < MAX_THREADS; i++) {
                    if (threads_idle[i]) {
                        thread_id = i;
                        threads_idle[i] = false;
                        break;
                    }
                }
                if (thread_id != -1) break;
                usleep(1000); // sleep for 1ms to avoid busy waiting
            }
        }

        // Assign the request to the selected thread
        pthread_create(&threads[thread_id], NULL, handleRequest, FD);
    }
}

string Server::handleChunkMsg(string & msg, int received_len, Client * client_class) {
    int client_fd = client_class->client_fd;
    size_t pos = msg.find("\n");
    int send_len = stoi(msg.substr(0,pos));
    int head_len = pos + sizeof("\n") - 1;
    string wholeRequest = msg.substr(head_len);
    // cout << "Receive length: " << wholeRequest.length() << endl;
    if (received_len != send_len + head_len) {//chunked
        cout << "msg is chunked" << endl;
        int total_content_len = wholeRequest.length();//now received
        while (total_content_len != send_len){
            char msg_continue[65536];
            int len = recv(client_fd, msg_continue, sizeof(msg_continue), 0);
            if (len <= 0) break;
            string temp(msg_continue, len);
            wholeRequest += temp;
            total_content_len += len;
        } 
        return wholeRequest;
    } else {
        cout << "msg is not chunked" << endl; 
        return wholeRequest;
    }
}

void * Server::handleRequest(void * client_class) {
    int client_fd = ((Client *)client_class)->client_fd;
    char buffer[65535];
    int len = recv(client_fd, buffer, sizeof(buffer), 0);
    cout << "Server received: \n" << buffer << endl;
    string received_msg(buffer, len);
    string wholeRequest = handleChunkMsg(received_msg, len, (Client *)client_class);

    XMLDocument request_doc;
    XMLError error_status = request_doc.Parse(wholeRequest.c_str());

    // check if the parsing was successful
    if (error_status != XML_SUCCESS) {
        error("server received invalid format");
        return NULL;
    } 
    // cout << "Server received pure request: \n" << request << endl;
   
    XMLElement * root = request_doc.RootElement();

    /*connect to database*/
    connection * C;
    connectDB(&C); 
    
    if (root == nullptr) {
    error("XML document has no root element");
    } else if (root->NextSiblingElement() != nullptr) {
    error("XML document has multiple root elements");
    } else if ((string(root->Name()).compare("transactions") == 0) && root->NoChildren()) {//create can have 0 children
    error("XML document(trasaction) has empty root element");
    } else if (string(root->Name()).compare("create") == 0) {
        cout << "XML create Document received" << endl;
        //pthread_mutex_lock(&mutex);
        handleCreate(root, C, (Client *)client_class);
        //pthread_mutex_unlock(&mutex);
    } else if (string(root->Name()).compare("transactions") == 0) {
        cout << "XML transactions Document received" << endl;
        //pthread_mutex_lock(&mutex);
        handleTransactions(root, C, (Client *)client_class);
        //pthread_mutex_unlock(&mutex);
    } else {
        error("XML Document root element is not regignized");
    }
    disconnectDB(C);

    // Mark the thread as idle
    int thread_id = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_equal(threads[i], pthread_self())) {
            thread_id = i;
            break;
        }
    }
    if (thread_id != -1) {
        threads_idle[thread_id] = true;
    }

    return NULL;
}

void Server::handleCreate(XMLElement * root, connection * C, Client * client_class) {
    //create results xml
    XMLDocument results_doc;
    createResultsXML(&results_doc);
    for (XMLElement * node = root->FirstChildElement(); node != NULL; node = node->NextSiblingElement()){
        if (string(node->Name()).compare("account") == 0){
            handleCreateAccount(node, &results_doc, C);
        } else if (string(node->Name()).compare("symbol") == 0) {
            handleCreateSymbol(node, &results_doc, C);
        } else {
            error("create request has node other than \"account\" and \"symbol\"");
        }
    }
    sendResultsToClient(&results_doc, client_class);
}


void Server::handleCreateAccount(XMLElement * account_node, XMLDocument * results_doc, connection * C) {
    long id = account_node->Int64Attribute ("id");
    double balance = account_node->DoubleAttribute("balance");
    // cout << "Account ID: " << id << ", balance: " << balance << endl;
    bool is_valid_account = checkAccount(id, C);//NO-TRUE
    if (is_valid_account && createAccount(id, balance, C)){
        cout << "Created account" << endl; 
        resultsSuccessCreateAccount(results_doc, id);
    } else {
        cout << "sending error msg create accout failed to client ..." << endl;
        resultsErrorCreateAccount(results_doc, id);
    }
}

void Server::handleCreateSymbol(XMLElement * symbol_node, XMLDocument * results_doc, connection * C) {
    string sym = symbol_node->Attribute("sym");
    // cout << "Symbol: " << sym << endl;

    // Extract the "account" element inside the "symbol" element
    XMLElement* account_node2 = symbol_node->FirstChildElement("account");
    if (account_node2 == NULL) {
        error("Symbol node must has at least one child account node");
    }
    while (account_node2) {//1 or more
        long id = account_node2->Int64Attribute("id");
        double num = account_node2->DoubleText();
        // cout << "  Account ID: " << id << ", value: " << num << endl;
    
        bool create_symbol = createPosition(sym, id, num, C);
        if (create_symbol) {//if create symbol succeed
            resultsSuccessCreateSymbol(results_doc, id, sym);  
        } else {//if create symbol failed
            resultsErrorCreateSymbol(results_doc, id, sym);
        }
        account_node2 = account_node2->NextSiblingElement();
    }
}

void Server::handleTransactions(XMLElement * root, connection * C, Client * client_class) {
    //create results xml
    XMLDocument results_doc;
    createResultsXML(&results_doc);
    long account_id = root->Int64Attribute("id");
    for (XMLElement * node = root->FirstChildElement(); node != NULL; node = node->NextSiblingElement()){
        if (string(node->Name()).compare("order") == 0){
            handleOrder(node, &results_doc, account_id, C);
        } else if (string(node->Name()).compare("query") == 0) {
            handleQuery(node, &results_doc, account_id, C);
        } else if (string(node->Name()).compare("cancel") == 0) {
            handleCancel(node, &results_doc, account_id, C);
        } else {
            error("create request has node other than \"account\" and \"symbol\"");
        }
    }
    sendResultsToClient(&results_doc, client_class);
}

void Server::handleOrder(XMLElement * order_node, XMLDocument * results_doc, long account_id, connection * C) {
    string sym = order_node->Attribute("sym");
    double amount = order_node->DoubleAttribute("amount");//negative = sell
    double limit = order_node->DoubleAttribute("limit");
    cout << "Order: sym = " << sym << ", amount = " << amount << ", limit = " << limit << endl;

    bool account_not_exist = (checkAccount(account_id, C));
    if (account_not_exist) {
        resultsErrorOrder(results_doc, sym, amount, limit, "Account not valid");
    } else {//valid account
        //TODO:call open order function
        // bool is_open = true;//TODO:
        // if (is_open) {
        //     int trans_id = 1;//TODO:call function to get trans_id
        //     resultsOpenedOrder(results_doc, sym, amount, limit, trans_id);
        // } else {
        //     resultsErrorOrder(results_doc, sym, amount, limit, "Order cannot be opened");
        // }
        processOrder(C, account_id, sym, amount, limit, results_doc);
    }

   
}

void Server::handleQuery(XMLElement * query_node, XMLDocument * results_doc, long account_id, connection * C) {
    long trans_id = query_node->Int64Attribute("id");
    cout << "Query: trans_id = " << trans_id << endl;
    bool account_not_exist = checkAccount(account_id, C);//TODO:
    if (account_not_exist) {
        resultsErrorNode(results_doc, trans_id, "query", "failed due to invalid account");
    } else {
        /*
        //TODO:call function to do get query information
        bool is_query_success = true;//TODO:
        if (is_query_success) {
            //TODO:call function to get status
        } else {
            resultsErrorNode(results_doc, trans_id, "query", "failed to get status");
        }
        */
       queryTrans(trans_id, C, results_doc, account_id);
    }
}

void Server::handleCancel(XMLElement * cancel_node, XMLDocument * results_doc, long account_id, connection * C) {
    long trans_id = cancel_node->Int64Attribute("id");
    cout << "Cancel: trans_id = " << trans_id << endl;
    bool account_not_exist = checkAccount(account_id, C);//TODO:
    if (account_not_exist) {
        resultsErrorNode(results_doc, trans_id, "cancel", "failed due to invalid account");
    } else {
        /*
        //TODO:call function to do cancels
        bool is_canceled = true;//TODO:
        if (is_canceled) {
            //TODO:call function to get cancel information(status without open)
        } else {
            resultsErrorNode(results_doc, trans_id, "cancel", "order failed");
        }
        */
       cancelResult(C, trans_id, results_doc, account_id);
    }
}

/*Results*/
void Server::createResultsXML(XMLDocument * doc) {
    const char * declaration_line = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    doc->Parse(declaration_line);
    XMLElement * root = doc->NewElement("results");
    doc->InsertEndChild(root);
}

void Server::sendResultsToClient(XMLDocument * doc, Client * client_class) {
    int client_fd = client_class->client_fd;
    XMLPrinter printer;
    doc->Print(&printer);
    string results = printer.CStr();
    cout << "sending results: \n" << results << endl;
    if (int len = send(client_fd, results.c_str(), results.length(), 0) == -1) {
        error("sending results failed");
    }
}


int main(void){
    cout << "Server started......." << endl;
    connection *C;
    connectDB(&C);
    //remove dropTable() when deploy, 
    //exist while developing to ensure the database empty when started
    ExecuteSql(DROP_TABLE,C);
    ExecuteSql(CREATE_TABLE,C);
    // createSymbol(C,"ABC");
    // createAccount(C,12,100);
    // long trans_id = insertTrans(C,12,"ABC",2,3);
    // cout<<trans_id<<endl;
    disconnectDB(C);
    delete C;

    Server server;
    server.create_server();
    server.run();
    return 0;
}