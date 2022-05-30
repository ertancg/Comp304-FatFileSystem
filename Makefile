NAME = minifs

FILES = $(shell basename -a $$(ls *.cpp) | sed 's/\.cpp//g')
SRC = $(patsubst %, %.cpp, $(FILES))
OBJ = $(patsubst %, %.o, $(FILES))
# HDR = $(patsubst %, -include %.h, $(FILES))
CXX = g++ -Wall

%.o : %.cpp
	$(CXX) -c -o $@ $<

build: $(OBJ)
	$(CXX) -o $(NAME) $(OBJ)

clean:
	rm -vf $(NAME) $(OBJ)
