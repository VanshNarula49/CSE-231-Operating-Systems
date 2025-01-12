all:
	gcc -m32 -g -Wall -o loader loader.c
	gcc -m32 -nostdlib -static -no-pie -o sum sum.c
	./loader ./sum

clean:
	rm -f loader sum
