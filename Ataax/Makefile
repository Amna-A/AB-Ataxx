CC := g++
CCOPTS=-Wall -Wextra -O3 -std=c++17

Ataxx_DEP_OBJ := Ataxx.o Board.o
Header:=Board.h

all: ataxx

%.o: %.C DEPS Header
	$(CC) ${CCOPTS}  -c -o $@ $<

ataxx: $(Ataxx_DEP_OBJ)
	${CC} ${CCOPTS} -o ataxx $^

clean:
	rm -f Ataxx *.o

	





	




