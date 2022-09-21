CXXFLAGS = -Wno-c++11-extensions

NRN_INCDIR=${N}/include
NRN_LIBDIR='"${N}/lib"'

nrn_test2: nrn_test2.cpp nrn_test2.h makefile
	$(CXX) $(CXXFLAGS) -I${NRN_INCDIR} -DNRN_LIBDIR=${NRN_LIBDIR} nrn_test2.cpp -o nrn_test2
