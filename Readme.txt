*******************************************************************
README 
for the package barn_test.

The package barn_test contains portable tools 
for concise, simple and small scale unit testing.


author: langenhagen (barn07@web.de)
version: 160126
*******************************************************************

CONTENTS:
    0. OVERVIEW
    1. USAGE
        1.1 FunctionTest
    2. TODO
    3. HISTORY


0. OVERVIEW #######################################################################################
###################################################################################################
  
The Solution holds 1 class:

    FunctionTest        :       function correctness tests
   
   
    - The doxygen documentation can be found in the folder "doc"
    
    
1. USAGE ##########################################################################################
###################################################################################################


1.1 FunctionTest ##################################################################################

// Write function tests as follows

#include <barn_test/FunctionTest.hpp>


// function to be tested
std::vector<int> fun(int i, int j) {
    return {1, i, j};
}

// comparison function to check actual and expected result
auto comp = [](const std::vector<int>& a, const std::vector<int>& b){ return std::equal(a.begin(), a.end(), b.begin(), b.end()); };
auto tostring = [](const std::vector<int>& v) { return to_string(v, ", "); };

unittest::FunctionTest<std::vector<int>, int, int> tester(fun, comp, tostring);

tester.test("Run 1", { 1,13,15 }, 13, 15);      // passes
tester.test("Run 2", { 1,13,15 }, 13, 99);      // fails

// ...

tester.write_test_series_summary();



// To test a method on an object, I'm afraid you have to wrap the method call into a lambda:

struct C {
    int foo(int i, int j) { return i + j; }  //< method to be tested 
};

C c;

auto fun = [&obj = c](int i, int j) { return obj.foo(i, j); };

FunctionTest<int, int, int> tester( fun);

// ...


2. TODO ###########################################################################################
###################################################################################################  


TODO test-functions for functions with side-effects. class StatusTest;
    if it really pays off
    lookat: http://stackoverflow.com/questions/16868129/how-to-store-variadic-template-arguments
    
TODO test-functions that test for exceptions. test_exceptions(...)

TODO test functions for speed
    lookat: http://stackoverflow.com/questions/255645/how-can-i-count-operations-in-c
    
TODO test-function that aggregates all test functions. test_function(...)

TODO review FunctionTest.hpp

TODO implement test_FunctionTest.cpp

       
3. HISTORY ########################################################################################
###################################################################################################    

160126      - added FunctionTest::write_test_series_summary()
            - changed FunctionTest::test() return type and added helper functions.
            - added file test_FunctionTest.cpp

160121      - some fine grained fixes on the FunctionTest class
            - enforced correct result type on FunctionTest::test()
            

# END OF FILE #