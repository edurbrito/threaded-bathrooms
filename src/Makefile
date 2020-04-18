.PHONY: all begin generate run clean

all: begin generate

begin:
		@echo "## Starting Compiling ##"

generate:
		@echo "Creating ouput files..."
		gcc U1.c utils.c logging.c -o U1.o -Wall -lpthread -D_REENTRANT
		gcc Q1.c utils.c logging.c -o Q1.o -Wall -lpthread -D_REENTRANT

runU: begin generate
		@echo "## Starting running U ##"
		./U1.o

runQ: begin generate
		@echo "## Starting running Q ##"
		./Q1.o

logU: begin generate
		./U1.o > ../logs/logU.txt

logQ: begin generate
		./Q1.o > ../logs/logQ.txt

clean:
		@echo "Cleaning up..."
		-rm U1.o Q1.o -f