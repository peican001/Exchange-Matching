#include "client.h"

int server_fd;

/*Make xml request*/
//return 0 on success
int createXML(const char * xmlPath, const char * root_name, int account_id) {
    const char * declaration_line = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    XMLDocument doc;
    doc.Parse(declaration_line);

    //Add root
    XMLElement * root = doc.NewElement(root_name);
    doc.InsertEndChild(root);

    if(strcmp(root_name, "transactions") == 0) {
        root->SetAttribute("id", account_id);
    }

    return doc.SaveFile(xmlPath);
}

//return 0 on success
int insertAccount(int account_id, float balance, const char * xmlPath) {
    XMLDocument doc;
    int status = doc.LoadFile(xmlPath);
    if (status != 0) {
        error("insert account: cannot open xmlfile");
    }
    XMLElement * root = doc.RootElement();
    XMLElement * accountNode = doc.NewElement("account");
    accountNode->SetAttribute("id", account_id);
    accountNode->SetAttribute("balance", balance);
    root->InsertEndChild(accountNode);

    return doc.SaveFile(xmlPath);
}

XMLElement * insertSymbol(string sym_name, const char * xmlPath) {
    XMLDocument doc;
    int status = doc.LoadFile(xmlPath);
    if (status != 0) {
        error("insert symbol: cannot open xmlfile");
    }
    XMLElement * root = doc.RootElement();
    XMLElement * symNode = doc.NewElement("symbol");
    symNode->SetAttribute("sym", sym_name.c_str());
    root->InsertEndChild(symNode);

    // insertAccountInSymbol(symNode, &doc, account_id, num);
    // insertAccountInSymbol(symNode, &doc, 66666, 588);
    // insertAccountInSymbol(symNode, &doc, 9999, 98);
    
    doc.SaveFile(xmlPath);
    return symNode;

}

int insertAccountInSymbol(XMLElement * symNode, int account_id, int num, const char * xmlPath) {
    XMLDocument doc;
    int status = doc.LoadFile(xmlPath);
    if (status != 0) {
        error("insert symbol: cannot open xmlfile");
    }
    XMLElement * accountNode = doc.NewElement("account");
    accountNode->SetAttribute("id", account_id);
    XMLText * accountNum = doc.NewText(to_string(num).c_str());
    accountNode->InsertEndChild(accountNum);
    symNode->InsertEndChild(accountNode);
    return doc.SaveFile(xmlPath);
}


int insertOrder(const char * xmlPath, string sym, int amount, int limit) {
    XMLDocument doc;
    int status = doc.LoadFile(xmlPath);
    if (status != 0) {
        error("insert order: cannot open xmlfile");
    }
    XMLElement * root = doc.RootElement();
    XMLElement * orderNode = doc.NewElement("order");
    orderNode->SetAttribute("sym", sym.c_str());
    orderNode->SetAttribute("amount", amount);
    orderNode->SetAttribute("limit", limit);
    root->InsertEndChild(orderNode);

    return doc.SaveFile(xmlPath);
}

int insertNodeTrans(const char * xmlPath, string node_name, int trans_id) {
     XMLDocument doc;
    int status = doc.LoadFile(xmlPath);
    if (status != 0) {
        error("insert node: cannot open xmlfile");
    }
    XMLElement * root = doc.RootElement();
    XMLElement * orderNode = doc.NewElement(node_name.c_str());
    orderNode->SetAttribute("id", trans_id);
    root->InsertEndChild(orderNode);

    return doc.SaveFile(xmlPath);
}

/*Send request to server*/
void error(const char * msg){
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

void connect_server(const char * hostname, const char * port){
    int status;
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
        error("player cannot create client socket");
    }
    //connect to ringmaster (ringmaster_fd must be listeing ... for layer to connect)
    status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        error("player cannot connect to server socket");
    }
    freeaddrinfo(host_info_list);
    cout << "Connected to " << hostname << " on port " << port << endl;
}

void send_request(const char * file) {
    string request_file(file);
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
}

void receive_results() {
    char buffer[65536];
    int len = recv(server_fd, buffer, sizeof(buffer), 0);
    cout << "Received: \n" << buffer << endl;
}

int main(int argc, char *argv[]) {
    // if (argc != 4) {
    //     error("input form should be <hostname> <port_num> <test_file>");
    // }
    if (argc != 2) {
        error("input form should be <test_file>");
    }
    const char * xmlPath = argv[1];

    /*creating xml file stored in argv[1]*/
    //-------Create-------
    // createXML(xmlPath, "create", 0);
    // insertAccount(123456, 1000, xmlPath);
    // XMLElement *  SPY = insertSymbol("SPY", xmlPath);
    // insertAccountInSymbol(SPY, 123456, 100000, xmlPath);
    // insertAccount(9999, 988.9, xmlPath);
    
    //-------Transactions------- 
    // createXML(xmlPath, "transactions", 2345);
    // insertOrder(xmlPath, "SPY", 300, 125);
    // insertOrder(xmlPath, "SPY", -100, 130);
    // insertOrder(xmlPath, "SPY", 200, 127);
    // insertOrder(xmlPath, "SPY", -500, 128);
    // insertOrder(xmlPath, "SPY", -200, 140);
    // insertOrder(xmlPath, "SPY", 400, 125);
    // insertNodeTrans(xmlPath, "query", 33333);
    // insertNodeTrans(xmlPath, "cancel", 33333);

    /*sending request of file argv[1]*/
    // connect_server(argv[1], argv[2]);//for real program
    //  connect_server("wille-virtual-machine", "12345");
    connect_server("vcm-32284.vm.duke.edu", "12345");//for testing
    send_request(xmlPath);
    receive_results();
    return 0;
}