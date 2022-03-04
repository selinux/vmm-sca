OUTPUT=sidechannel-VMM

run: $(OUTPUT)
	./sidechannel-VMM -l

$(OUTPUT): $(OUTPUT).o  ../guests/payload.o ../common/common.h
	$(CC) $^ -o $@

$(OUTPUT).o: $(OUTPUT).c ../guests/payload.o
	$(CC) $< -c

../guests/payload.o: ../common/common.h
	$(MAKE) -C ../guests CC_DEFINES=$(CC_DEFINES) CC_FLAGS="$(CC_FLAGS)" LD_FLAGS="$(LD_FLAGS)"

common/common.h:


.PHONY: clean
clean:
	$(RM) *.o
	$(MAKE) -C ../guests clean
