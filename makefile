all: AntiVirus

AntiVirus: AntiVirus.o
	gcc -g -m32 -Wall -o AntiVirus AntiVirus.o

AntiVirus.o: AntiVirus.c
	gcc -g -m32 -Wall -c -o AntiVirus.o AntiVirus.c


bubblesort: lab3_bubblesort.o
	gcc -g -m32 -Wall -o bubblesort lab3_bubblesort.o

lab3_bubblesort.o: lab3_bubblesort.c
	gcc -g -m32 -Wall -c -o lab3_bubblesort.o lab3_bubblesort.c

.PHONY: clean

clean:
	rm -f *.o bubblesort AntiVirus
