all: stnc server client

stnc: stnc.c
	gcc -o stnc stnc.c

server: Server.c NC_Utils.c
	gcc -o server Server.c

client: Client.c NC_Utils.c
	gcc -o client Client.c