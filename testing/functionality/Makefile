TARGETS=client
EXTRAFLAGS=-lpqxx -lpq -lpthread -pthread
all: $(TARGETS)
clean:
	rm -f $(TARGETS)

client: client.cpp
	g++ -g -o $@ $< tinyxml2.cpp
