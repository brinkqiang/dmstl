include ../makefile.comm

OBJECTS = obj/gallocator.o \

			
			
			
			
			


LIB_TARGET = lib/libmyallocator.a

INC_DIR= include/
SRC_DIR= src/
OBJ_DIR= obj/
LIB_DIR= lib/

OBJ_EXT= .o
CXXSRC_EXT= .cpp
CSRC_EXT= .c

$(OBJ_DIR)%$(OBJ_EXT): $(SRC_DIR)%$(CXXSRC_EXT)
	@echo
	@echo "Compiling $< ==> $@..."
	$(CXX) $(INC) -I$(INC_DIR) $(C_FLAGS) -c $< -o $@

$(OBJ_DIR)%$(OBJ_EXT): $(SRC_DIR)%$(CSRC_EXT)
	@echo
	@echo "Compiling $< ==> $@..."
	$(CXX) $(INC) $(C_FLAGS) -c $< -o $@

$(LIB_TARGET): $(OBJECTS)
	@echo
	@echo "$(OBJECTS) ==> $@"
	$(AR) rc $(LIB_TARGET) $(OBJECTS)

all: $(LIB_TARGET)

clean:
	rm -f $(LIB_TARGET) $(OBJECTS)
