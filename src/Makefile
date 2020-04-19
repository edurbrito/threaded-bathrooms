.PHONY: all begin generate run clean

all: begin generate

begin:
		@echo "## Starting Compiling ##"

generate:
		@echo "Creating ouput files..."
		gcc U1.c utils.c logging.c -o U1 -Wall -lpthread -D_REENTRANT
		gcc Q1.c utils.c logging.c -o Q1 -Wall -lpthread -D_REENTRANT

runU: begin generate
		@echo "## Starting running U ##"
		./U1

runQ: begin generate
		@echo "## Starting running Q ##"
		./Q1

logU: begin generate
		./U1 > ../logs/logU.txt

logQ: begin generate
		./Q1 > ../logs/logQ.txt

clean:
		@echo "Cleaning up..."
		-rm U1 Q1 -f