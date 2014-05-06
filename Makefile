CC = g++
LOCAL = -D NO_DEVICE  -D DEBUG
DEV = -D DEBUG
OBJ= N1470.o test.o
INC = -I libftdi -I/home/morgan/lib/d2xx
LIBDIRS = -L /usr/local/lib -L /home/morgan/lib/d2xx/build/x86_64
LIBS = -l ftd2xx
CFLAGS = -c -Wall

test: $(OBJ)
	$(CC) $(LIBDIRS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC) $(DEV) $<
		
