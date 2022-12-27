# Настройки компилятора
CC = gcc
CFLAGS = --std=c17 -Wall -pedantic -Isrc/ -ggdb -Wextra -Werror -DDEBUG

# Папки
BUILDDIR = build
SRCDIR = src

# Файлы
EXECUTABLE = malloc_exe

all: build clean $(EXECUTABLE) test

$(EXECUTABLE): $(BUILDDIR)/util.o $(BUILDDIR)/mem.o $(BUILDDIR)/mem_debug.o $(BUILDDIR)/tests.o $(BUILDDIR)/main.o
	$(CC) -o $(BUILDDIR)/$@ $^ $(CFALGS)

$(BUILDDIR)/mem.o: $(SRCDIR)/mem.c $(SRCDIR)/mem.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/mem_debug.o: $(SRCDIR)/mem_debug.c $(SRCDIR)/mem_debug.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/util.o: $(SRCDIR)/util.c $(SRCDIR)/util.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/main.o: $(SRCDIR)/main.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/tests.o: $(SRCDIR)/tests.c $(SRCDIR)/tests.h
	$(CC) -c $(CFLAGS) $< -o $@
	
build:
	mkdir -p $(BUILDDIR)
	
.PHONY: clean

clean:
	rm -rf $(BUILDDIR)/*
	
test:
	./$(BUILDDIR)/$(EXECUTABLE)
	
