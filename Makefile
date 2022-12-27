# Настройки компилятора
CC = gcc
CFLAGS = --std=c17 -Wall -pedantic -I src/ -ggdb -Wextra -Werror -DDEBUG

# Папки
BUILDDIR = build
SRCDIR = src

# Файлы
RES = output.txt
EXEC = malloc_exe
SRC = $(shell find $(SRCDIR) -name *.c)
INC = $(shell find $(SRCDIR) -name *h)
OBJ = $(SRC:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)


all: build clean $(EXEC) test

$(EXEC): $(OBJ)
	$(CC) -o $(BUILDDIR)/$@ $^ $(CFALGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INC)
	$(CC) -c $(CFLAGS) $< -o $@
	
build:
	mkdir -p $(BUILDDIR)
	
.PHONY: clean

clean:
	rm -rf $(BUILDDIR)/* $(RES)
	
test:
	./$(BUILDDIR)/$(EXEC) 2>> $(RES)
	
