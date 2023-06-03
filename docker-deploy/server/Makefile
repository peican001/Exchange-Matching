TARGETS=server
EXTRAFLAGS=-lpqxx -lpq -lpthread
all: $(TARGETS)
clean:
	rm -f $(TARGETS)

server: server.cpp 
	g++ -g -o $@ $< tinyxml2.cpp db_funcs.cpp results.cpp $(EXTRAFLAGS)
