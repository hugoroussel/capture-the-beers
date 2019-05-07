# tools
CC := gcc
CFLAGS = -I$(IDIR) -Wall -Wextra -g -fno-stack-protector -lpthread -std=gnu11
#TO ADD BEFORE DEADLINE -z execstack
IDIR = ./include
ODIR = ./obj
BDIR_S = ./bin/server
BDIR_C = ./bin/client
SDIR = ./src
DEPS = functions.h grass.h server_commands.h


_DEPS = functions.h grass.h server_commands.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))


%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $(ODIR)/$@ $< $(CFLAGS)


default: server.o grass.o functions.o client.o server_commands.o
	$(CC) -o $(BDIR_S)/server $(ODIR)/server.o $(ODIR)/functions.o $(ODIR)/grass.o $(ODIR)/server_commands.o $(CFLAGS)
	$(CC) -o $(BDIR_C)/client $(ODIR)/client.o $(ODIR)/functions.o $(ODIR)/grass.o $(CFLAGS)


.PHONY: clean

clean:
	rm -f $(ODIR)/*.o  
