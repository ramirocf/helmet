CC=gcc
CCXFLAGS=-Wall -c
LD=ld 
LDXFLAGS=-O
LIBS=-l bcm2835 -pthread -lm -lwiringPi

all: helmet

temps_and_humidity.o: temps_and_humidity.c
	$(CC) $(CCXFLAGS) temps_and_humidity.c -o temps_and_humidity.o

ADS1015.o: ADS1015.c
	$(CC) $(CCXFLAGS) ADS1015.c -o ADS1015.o

Pulse_Oximeter.o: Pulse_Oximeter.c
	$(CC) $(CCXFLAGS) Pulse_Oximeter.c -o Pulse_Oximeter.o

i2cUVsensor.o: i2cUVsensor.c 
	$(CC) $(CCXFLAGS) i2cUVsensor.c -o i2cUVsensor.o

main.o: main.c
	$(CC) $(CCXFLAGS) main.c -o main.o 

helmet: main.o temps_and_humidity.o ADS1015.o Pulse_Oximeter.o i2cUVsensor.o 
	$(CC) $(LDXFLAGS) -o helmet main.o temps_and_humidity.o ADS1015.o Pulse_Oximeter.o i2cUVsensor.o $(LIBS)

clean:
	rm *.o
	rm helmet