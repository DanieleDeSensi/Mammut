TARGET               = joules joulesCpu voltageTable demo-energy-extended activationPower launcher

.PHONY: all clean cleanall

all: $(TARGET)

%: %.cpp $(MAMMUTROOT)/mammut/libmammut.a
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCS) $(LDFLAGS) $(LDLIBS)
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ 
	-rm -fr $(TARGET)
