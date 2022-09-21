#include <dlfcn.h>
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include "nrn_test2.h"

using std::cout;
using std::endl;
using std::exit;

typedef void (*initer_function)(int, const char**, const char**, int64_t);
typedef void (*vd_function)(double);
typedef void (*vv_function)(void);
typedef int64_t (*icptr_function)(const char*);
typedef void* (*vcptr_function)(const char*);
typedef Symbol* (*scptr_function)(const char*);
typedef double (*dvptrint_function) (void*, int64_t);
typedef Symbol* (*scptroptr_function) (char*, Object*);
typedef double (*dsio_function) (Symbol*, int64_t, Object*);
typedef Symbol* (*scptrslptr_function) (const char*, Symlist*);


static const char* argv[] = {"nrn_test", "-nogui", "-nopython", NULL};



extern "C" void modl_reg() {};


int main(void) {
    char* error;
    Symbol* sym;
    void* handle = dlopen(NRN_LIBDIR "/libnrniv.dylib", RTLD_NOW | RTLD_LOCAL); 
    if (!handle) {
        cout << "Couldn't open dylib." << endl << dlerror() << endl;
        exit(-1);
    } else {
        cout << "Opened dylib" << endl;
    }

    /***************************
     * 
     * A bunch of functions in NEURON we'll want to be able to call
     * 
     **************************/

    // just using hoc_last_init alone is insufficient, which is too bad because it isn't mangled
    auto ivocmain = (initer_function) dlsym(handle, "_Z16ivocmain_sessioniPPKcS1_i");  // mangled version of: ivocmain_session
    error = dlerror();
    assert(!error);

    *((int64_t*) dlsym(handle, "nrn_main_launch")) = 0;

    auto hoc_lookup = (scptr_function) dlsym(handle, "hoc_lookup");
    assert(hoc_lookup);

    auto hoc_table_lookup = (scptrslptr_function) dlsym(handle, "hoc_table_lookup");
    assert(hoc_table_lookup);

    auto hoc_call_func = (dvptrint_function) dlsym(handle, "hoc_call_func");
    assert(hoc_call_func);

    auto hoc_oc = (icptr_function) dlsym(handle, "hoc_oc");
    assert(hoc_oc);

    auto hoc_pushx = (vd_function) dlsym(handle, "hoc_pushx");
    assert(hoc_pushx);

    auto hoc_ret = (vv_function) dlsym(handle, "hoc_ret");
    assert(hoc_ret);

    auto hoc_call_objfunc = (dsio_function) dlsym(handle, "hoc_call_objfunc");
    assert(hoc_call_objfunc);

    auto hoc_top_level_data = (Objectdata*) dlsym(handle, "hoc_top_level_data");
    assert(hoc_top_level_data);

    /***************************
     * 
     * Miscellaneous initialization
     * 
     **************************/

    // commenting out the following line shows the banner
    *((int64_t*) dlsym(handle, "nrn_nobanner_")) = 1;
    
    ivocmain(3, argv, NULL, 0);





    /***************************
     * run HOC code
     **************************/
    hoc_oc(
        "create soma\n"
    );

    cout << "created the soma; now lets look at topology:" << endl;    

    /***************************
     * lookup a symbol and call the corresponding function with 0 arguments
     **************************/
    sym = hoc_lookup("topology");
    hoc_call_func(sym, 0);  // the 0 is the number of args; returns the return of the function (1)


    /***************************
     * call finitialize, pass in an initial voltage (so 1 argument instead of 0)
     **************************/
    sym = hoc_lookup("finitialize");
    hoc_pushx(3.14);
    hoc_call_func(sym, 1);

    /***************************
     * lookup a top-level symbol and set the value (equivalent to t=1.23)
     **************************/
    sym = hoc_lookup("t");
    *(sym->u.pval) = 1.23;


    /***************************
     * call fadvance -- segfaults
     **************************/
    //sym = hoc_lookup("fadvance");
    //assert(sym);
    //hoc_call_func(sym, 0);

    /***************************
     * call fadvance -- also segfaults, but gives a backtrace suggesting it fails in nrn_fixed_step
     **************************/
    // hoc_oc("fadvance()\n");

    /***************************
     * print out the time and the voltage (print, alas, is a statement not a function)
     **************************/
    cout << "time and voltage:" << endl;
    hoc_oc("print t, v\n");

    /***************************
     * create a Vector via HOC, print out contents via C++
     **************************/
    hoc_oc(
        "objref myVec\n"
        "myVec = new Vector(5)\n"
        "objref myVec2\n"
        "myVec2 = new Vector(10)\n"
        "print \"created myVec and myVec2\"\n"
    );
    /*
    hoc_oc(
        "myVec.x[0] = 0\n"
        "myVec.x[1] = 1\n"
        "myVec.x[2] = 22\n"
        "myVec.x[3] = 333\n"
        "myVec.x[4] = 4444\n"
    );
    */



    // getting memory locations from outside seems difficult as v_as_numpy gets that from vec->data()
    // but outsiders don't have access to the C++ whatevers
    auto myVec = hoc_lookup("myVec");
    auto myVec2 = hoc_lookup("myVec2");
    assert(myVec);
    cout << "myVec at " << myVec << endl;
    cout << "myVec->type " << myVec->type << endl;
    cout << "myVec->u.ctemplate " << myVec->u.ctemplate << endl;
    cout << "myVec2->name " << myVec2->name << endl;
    cout << "myVec2->u.ctemplate " << myVec2->u.ctemplate << endl;
    Object** myVec_object_ptr = hoc_top_level_data[myVec->u.oboff].pobj;
    cout << "myVec_object_ptr " << myVec_object_ptr << endl;
    auto myVec_object = *myVec_object_ptr;
    auto myVec_set = hoc_table_lookup("set", myVec->u.ctemplate->symtable);
    hoc_pushx(1);
    hoc_pushx(2);
    cout << "time for hoc_call_objfunc to object at " << myVec_object << endl;
    hoc_call_objfunc(myVec_set, 2, myVec_object);

    cout << "trying printf a different way" << endl;
    
    sym = hoc_lookup("printf");
    assert(sym);
    hoc_call_objfunc(sym, 0, myVec->u.object_);


}
