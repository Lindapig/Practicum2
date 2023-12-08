all: rfs rfserver

rfs: client.c helper.c helper.h
	gcc -o rfs client.c helper.c -lpthread

rfserver: server.c helper.c helper.h
	gcc -o rfserver server.c helper.c -lpthread

clean:
	rm -f rfs rfserver