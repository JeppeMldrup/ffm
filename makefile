install:
	gcc -lncurses ffm.c -o /bin/ffm
debug:
	gcc -g -lncurses ffm.c -o /bin/ffm
