all:
	gcc -c file_hiding_helper.c -g
	gcc -c file_hiding.c -g
	gcc -o slackFS file_hiding.o file_hiding_helper.o -lerasurecode -ldl -Wall -g
	gcc -shared -o libslackFS.so -fPIC file_hiding_helper.c -lerasurecode -ldl
	rm *.o

clean:
	rm slackFS libslackFS.so
