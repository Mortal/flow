CXXFLAGS += -march=native -O3 -std=c++0x -Wall -Wextra
LIBS := -lboost_program_options -lboost_system -lboost_chrono
LDFLAGS += $(LIBS)

all: height

clean:
	$(RM) height
