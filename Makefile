CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic
OBJDIR := obj
SOURCES := main.c
OBJECTS := $(SOURCES:%.c=$(OBJDIR)/%.o)
TARGET := trabalho

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) -lm

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -Iheaders -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
