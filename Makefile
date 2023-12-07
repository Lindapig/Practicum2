all: rfs rfserver

rfs: client.c helpers.c helpers.h
	gcc -o rfs client.c helpers.c -lpthread

rfserver: server.c helpers.c helpers.h
	gcc -o rfserver server.c helpers.c -lpthread

clean:
	rm -f rfs rfserver