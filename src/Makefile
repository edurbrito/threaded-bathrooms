.PHONY: all begin generate run clean

all: begin generate

begin:
		@echo "## Starting Compiling ##"

generate:
		@echo "Creating ouput files..."
		gcc U2.c utils.c -o U2 -Wall -lpthread -D_REENTRANT
		gcc Q2.c utils.c -o Q2 -Wall -lpthread -D_REENTRANT

clean:
		@echo "Cleaning up..."
		-rm U2 Q2 -f
		-rm *.log *.err -f

test:
		@./testing.sh