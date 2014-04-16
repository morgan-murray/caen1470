CC = g++
LOCAL = -D NO_DEVICE  -D DEBUG
OBJ= N1470.o test.o
INC = -I libftdi
LIBDIRS = -L /usr/local/lib
LIBS = -l ftd2xx
CFLAGS = -c -Wall

test: $(OBJ)
	$(CC) $(LIBDIRS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC) $(LOCAL) $<
		
