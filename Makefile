# Sources:
SRCS:=father.c
OBJS:=$(SRCS:.c=.o)

# Config:
CC:=gcc
CFLAGS:= -c
LD:=gcc

# Targets:

all: father

clean:
	@echo Cleaning.
	@rm -f *.o
	@rm -f father

father: $(OBJS)
	@echo $@
	@$(LD) -o $@ $^


%.o:%.c
	@echo $@
	@ $(CC) $(CFLAGS) -o $@ $<
