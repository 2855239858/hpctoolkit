INC = -I.. -I../,,
LIB = ../.libs/libHPCprof-lean.a
OFLAGS=-g

OBJS = test2  test

OMP=-fopenmp


all: $(OBJS)


test2: test2.o $(LIB)
	gcc -o test2 $(OFLAGS) test2.o $(LIB)

test: test.o $(LIB)
	gcc $(OMP) -o test $(OFLAGS) test.o $(LIB)

%.o: %.c
	gcc -c $(OFLAGS) $(INC) $(OMP) $<

%.i: %.c
	gcc -E $(INC) $< > $@

clean:
	rm -f $(OBJS) *.o
