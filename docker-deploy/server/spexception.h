#ifndef __SPEXCEPTION_H__
#define __SPEXCEPTION_H__

#include <exception>
#include <stdexcept>
#include <string>
using namespace std;

class spException : public std::exception {
public:
    std::string errMsg;
    spException(const std::string &errDescr, const std::string &Info){
        errMsg.append(errDescr);
        errMsg.append(Info);
        errMsg.append(" ");
    }
    //virtual ~MyException() _NOEXCEPT{}
    virtual const char * what() const throw() {
        return errMsg.c_str();
    }
};

class VersionErrorException : public std::exception {
 public:
  VersionErrorException() : message("Sql transaction fail. Beacuse:  ") {}
  VersionErrorException(string str) : message("Sql transaction fail. Beacuse:  " + str) {}
  ~VersionErrorException() throw() {}

  virtual const char * what() const throw() { return message.c_str(); }

 private:
  string message;
};

#endif