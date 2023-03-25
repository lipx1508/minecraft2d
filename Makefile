LDFLAGS  := -std=c++17 -Wall -mwindows
SOURCE   := $(wildcard src/*.cpp)
OUTPUT   := $(patsubst src/%.cpp, bin/%.o, $(SOURCE))
TARGET   := main

ifdef OS
	INCLUDES := -Iinclude
	LIBS     := -L. -lm -lmuffin -lmingw32
else
	LIBS     := -lm -lmuffin
endif

.PHONY: all link run clean

all: $(OUTPUT) link run

bin/%.o: src/%.cpp
	@echo '[ Building $<... ]'
	g++ $(LDFLAGS) -c $^ -o $@ $(INCLUDES) $(LIBS)

link:
	@echo '[ Linking... ]'
	g++ $(LDFLAGS) $(OUTPUT) -o $(TARGET) $(INCLUDES) $(LIBS)

run:
	@echo '[ Running... ]'
	./$(TARGET)

clean:
	@echo '[ Cleaning... ]'
	rm -fr bin/*.o