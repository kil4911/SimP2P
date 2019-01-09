
hellomake: ${SOURCE_FILE}
	/usr/bin/mpicc client.c -o client
	/usr/bin/mpicc server.c -o server

clean:
	rm client server

run_client: peer.h
	./client

run_server_local: peer.h
	/usr/bin/mpiexec -n 1 ./server

run_server_cluster: peer.h
	/usr/bin/mpiexec -n 4 --hostfile ./hostfile /SimP2P/./server