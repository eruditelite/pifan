ifdef DEBUG_BUILD
CFLAGS = -Wall -g
LIBS = -lpigpiod_if2 -lrt -lpthread
else
CFLAGS = -Wall -O2
LIBS = -lpigpiod_if2 -lrt -lpthread
endif

all: pifan

pifan: main.c fan.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

fan.o: fan.c fan.h
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -f *~ *.o pifan
