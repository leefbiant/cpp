include ../Makefile.in
Target = Proxy
CXXFLAGS +=  -D__CONN__  $(CINC)

LIB +=  -liconv

all:$(Target)

$(Target):$(OBJECTS)
	$(CC) -o $@ $^ $(LIB) $(LIBPATH)  $(LDFLAGS)
$(OBJECTS):%.o:%.cpp
	$(CC) -c $(CXXFLAGS) $<  -o $(OBJ_DIR)/$@

install: 
	cp -f $(Target) ../bin/proxy/$(Target)
.PHONY:clean
clean:
	rm -rf $(Target) $(OBJ_DIR)/*.o

