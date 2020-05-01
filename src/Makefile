.PHONY: all begin generate run clean

all: begin generate

begin:
		@echo "## Starting Compiling ##"

generate:
		@echo "Creating ouput files..."
		gcc U1.c utils.c -o U1 -Wall -lpthread -D_REENTRANT
		gcc Q1.c utils.c -o Q1 -Wall -lpthread -D_REENTRANT

clean:
		@echo "Cleaning up..."
		-rm U1 Q1 -f
		-rm *.log *.err -f

test:
		@./testing.sh