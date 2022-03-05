OUTPUT=sidechannel-VMM
CFLAGS =-g -Wall -Wextra -Werror -fno-stack-protector -O3

run: $(OUTPUT) ../guests/payload.o
	./$(OUTPUT)

$(OUTPUT): $(OUTPUT).o  ../guests/payload.o ../common/common.h
	$(CC) $^ -o $@

$(OUTPUT).o: $(OUTPUT).c ../guests/payload.o  ../common/common.h
	$(CC) $< -c

../guests/payload.o: ../common/common.h
	$(MAKE) -C ../guests payload.o CC_DEFINES=$(CC_DEFINES) CC_FLAGS="$(CC_FLAGS)" LD_FLAGS="$(LD_FLAGS)"

common/common.h:


.PHONY: clean
clean:
	$(RM) *.o $(OUTPUT)
	$(MAKE) -C ../guests clean
