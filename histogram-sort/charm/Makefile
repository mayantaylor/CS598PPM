-include ../../../common.mk
CHARMC=charmc $(OPTS)

OBJS = scanner.o

all: scanner

scanner: $(OBJS)
	$(CHARMC) -language charm++ -o scanner $(OBJS)

scanner.decl.h: scanner.ci
	$(CHARMC)  scanner.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o scanner charmrun

scanner.o: scanner.C scanner.decl.h
	$(CHARMC) -c scanner.C

test: all
	$(call run, ./scanner +p4 10 )

testp: all
	$(call run, ./scanner +p$(P) $$(( $(P) * 10 )) )
