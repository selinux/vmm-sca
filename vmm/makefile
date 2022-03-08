OUTPUT=VMM
CFLAGS =-g -lpthread -Wall -Wextra -Werror -fno-stack-protector -O3

run: $(OUTPUT) ../guests/payload.o
	./$(OUTPUT)

$(OUTPUT): $(OUTPUT).o VMM-thread.o ../guests/payload.o ../common/common.h ksm.o
	$(CC) $^ -o $@ $(CFLAGS)

$(OUTPUT).o: $(OUTPUT).c ../guests/payload.o  ../common/common.h
	$(CC) $< -c $(CFLAGS)

VMM-thread.o: VMM-thread.c VMM-thread.h ksm.o
	$(CC) $< -c $(CFLAGS)

ksm.o: ksm.c ksm.h
	$(CC) $< -c $(CFLAGS)

../guests/payload.o: ../common/common.h
	$(MAKE) -C ../guests payload.o CC_DEFINES=$(CC_DEFINES) CC_FLAGS="$(CC_FLAGS)" LD_FLAGS="$(LD_FLAGS)"

common/common.h:


.PHONY: clean
clean:
	$(RM) *.o $(OUTPUT)
	$(MAKE) -C ../guests clean
