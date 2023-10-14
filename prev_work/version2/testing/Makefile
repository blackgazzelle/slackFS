all:
	gcc -c slackFS.c -g
	gcc -c slackFS_driver.c -g
	gcc -o slackFS slackFS.o slackFS_driver.o -lerasurecode -ldl -Wall -g
	gcc -shared -o libslackFS.so -fPIC slackFS.c -lerasurecode -ldl
	rm *.o

clean:
	rm slackFS libslackFS.so
