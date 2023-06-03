#include "results.h"

void resultsSuccessCreateAccount(XMLDocument * doc, long account_id) {
    XMLElement * root = doc->RootElement();
    XMLElement * accountNode = doc->NewElement("created");
    accountNode->SetAttribute("id", account_id);
    root->InsertEndChild(accountNode);
}

void resultsSuccessCreateSymbol(XMLDocument * doc, long account_id, string sym) {
    XMLElement * root = doc->RootElement();
    XMLElement * symNode = doc->NewElement("created");
    symNode->SetAttribute("sym", sym.c_str());
    symNode->SetAttribute("id", account_id);
    root->InsertEndChild(symNode);
}
void resultsErrorCreateAccount(XMLDocument * doc, long account_id) {
    XMLElement * root = doc->RootElement();
    XMLElement * accountNode = doc->NewElement("error");
    accountNode->SetAttribute("id", account_id);
    XMLText * msg = doc->NewText("Account creation failed");
    accountNode->InsertEndChild(msg);
    root->InsertEndChild(accountNode);
}

void resultsErrorCreateSymbol(XMLDocument * doc, long account_id, string sym) {
    XMLElement * root = doc->RootElement();
    XMLElement * symNode = doc->NewElement("error");
    symNode->SetAttribute("sym", sym.c_str());
    symNode->SetAttribute("id", account_id);
    XMLText * msg = doc->NewText("Symbol creation failed");
    symNode->InsertEndChild(msg);
    root->InsertEndChild(symNode);
}

void resultsErrorOrder(XMLDocument * doc, string sym, double amount, double limit, string error_msg) {
    XMLElement * root = doc->RootElement();
    XMLElement * orderNode = doc->NewElement("error");
    orderNode->SetAttribute("sym", sym.c_str());
    orderNode->SetAttribute("amount", amount);
    orderNode->SetAttribute("limit", limit);
    XMLText * msg = doc->NewText(error_msg.c_str());
    orderNode->InsertEndChild(msg);
    root->InsertEndChild(orderNode);
}

void resultsErrorNode(XMLDocument * doc, long trans_id, string node_name, string error_msg) {
    XMLElement * root = doc->RootElement();
    XMLElement * Node = doc->NewElement("error");
    Node->SetAttribute("id", trans_id);
    string whole_msg = node_name + " " + error_msg;
    XMLText * msg = doc->NewText(whole_msg.c_str());
    Node->InsertEndChild(msg);
    root->InsertEndChild(Node);
}

void resultsOpenedOrder(XMLDocument * doc, string sym, double amount, double limit, long trans_id) {
    XMLElement * root = doc->RootElement();
    XMLElement * orderNode = doc->NewElement("opened");
    orderNode->SetAttribute("sym", sym.c_str());
    orderNode->SetAttribute("amount", amount);
    orderNode->SetAttribute("limit", limit);
    orderNode->SetAttribute("id", trans_id);
    root->InsertEndChild(orderNode);
}

XMLElement * resultsAddStatus(XMLDocument * doc, long trans_id) {
    XMLElement * root = doc->RootElement();
    XMLElement * statusNode = doc->NewElement("status");
    statusNode->SetAttribute("id", trans_id);
    root->InsertEndChild(statusNode);
    return statusNode;
}

void resultsStatusOpen(XMLDocument * doc, XMLElement * node, double shares) {
    XMLElement * openNode = doc->NewElement("open");
    openNode->SetAttribute("shares", shares);
    node->InsertEndChild(openNode);
}

void resultsStatusCanceled(XMLDocument * doc, XMLElement * node, double shares, long time) {
    XMLElement * canceledNode = doc->NewElement("canceled");
    canceledNode->SetAttribute("shares", shares);
    canceledNode->SetAttribute("time", time);
    node->InsertEndChild(canceledNode);
}
void resultsStatusExecuted(XMLDocument * doc, XMLElement * node, double shares, double price, long time) {
    XMLElement * exeNode = doc->NewElement("executed");
    exeNode->SetAttribute("shares", shares);
    exeNode->SetAttribute("price", price);
    exeNode->SetAttribute("time", time);
    node->InsertEndChild(exeNode);
}

XMLElement * resultsAddCanceled(XMLDocument * doc, long trans_id) {
    XMLElement * root = doc->RootElement();
    XMLElement * statusNode = doc->NewElement("canceled");
    statusNode->SetAttribute("id", trans_id);
    root->InsertEndChild(statusNode);
    return statusNode;
}