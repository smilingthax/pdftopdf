SOURCES=pdftopdf.cc pdftopdf_jcl.cc pdftopdf_processor.cc qpdf_pdftopdf_processor.cc pptypes.cc nup.cc intervalset.cc qpdf_tools.cc qpdf_xobject.cc qpdf_pdftopdf.cc
EXEC=pt

#CXX=/home/thobi/dls/gstlfilt/gfilt
#CFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
CFLAGS=-Wall -g
CXXFLAGS=-std=c++0x
LDFLAGS=-g
CPPFLAGS=$(CFLAGS) $(FLAGS)
PACKAGES=

ifneq "$(PACKAGES)" ""
  CPPFLAGS+=$(shell pkg-config --cflags $(PACKAGES))
  LDFLAGS+=$(shell pkg-config --libs $(PACKAGES))
endif
QPDF=/home/thobi/src/gsoc/2012/qpdf
CPPFLAGS+=-I$(QPDF)/include
LDFLAGS+=-L$(QPDF)/libqpdf/build -lqpdf

CPPFLAGS+=$(shell cups-config --cflags) 
LDFLAGS+=$(shell cups-config --libs)

OBJECTS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).o,\
        $(patsubst %.cc,$(PREFIX)%$(SUFFIX).o,\
$(SOURCES)))
DEPENDS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).d,\
        $(patsubst %.cc,$(PREFIX)%$(SUFFIX).d,\
        $(filter-out %.o,""\
$(SOURCES))))

all: $(EXEC)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(DEPENDS)
endif 

clean:
	rm -f $(EXEC) $(OBJECTS) $(DEPENDS)

%.d: %.c
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

%.d: %.cc
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

$(EXEC): $(OBJECTS)
#	$(CXX) -o $@ $^ $(LDFLAGS)
	libtool --mode=link $(CXX) -o $@ $^ $(LDFLAGS)
