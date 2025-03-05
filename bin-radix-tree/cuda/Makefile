
all: main

internal.o: internal.cu
	nvcc -c internal.cu -o internal.o

main: tree.cpp internal.o
	g++ tree.cpp internal.o -o main -lcudart

clean:
	rm -f *.o main