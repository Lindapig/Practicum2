all: rfs rfserver

rfs: client.c helpers.c helpers.h
	gcc -o rfs client.c helpers.c

rfserver: server.c helpers.c helpers.h
	gcc -o rfserver server.c helpers.c

clean:
	rm -f rfs rfserver