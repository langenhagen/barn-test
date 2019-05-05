*******************************************************************
README
for the package barn_test.

The package barn_test contains portable tools
for concise, simple and small scale unit testing.


author: langenhagen (barn07@web.de)
version: 160205
*******************************************************************

CONTENTS:
    0. OVERVIEW
    1. USAGE
        1.1 FunctionTest
        1.2 RandomizedFunctionTest
    2. TODO
    3. HISTORY


0. OVERVIEW #######################################################################################
###################################################################################################

The Solution holds 4 modules:

    FunctionTest                :       function correctness tests
    RandomizedFunctionTest      :       function tested against reference function multiple times
    verbosity                   :       enum class for specifying the verbosity of the logging.
    tuple_to_stream             :       utility function for writing tuples to an ostream

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

unittest::FunctionTest<int, int, int> tester( fun);

// ...


1.2 RandomizedFunctionTest ########################################################################

//A simple example

#include <barn_test/RandomizedFunctionTest.hpp>


string fun(float f, int i);             // function to be tested
string reference_fun(float f, int i);   // reference function

// ...

// argument tuple creation function - aways takes an unsigned int as argument
auto arg_creator = [](unsigned int i) { return tuple<float, int>(rand() % 10 *0.1f, rand() % 100); };

unittest::RandomizedFunctionTest<string, float, int> tester(fun, reference_fun, arg_creator);

auto test_result = tester.test("Test Run 1", 100);

// ...


// A complex example of the usage of the RandomizedFunctionTest is shown in the following.
// It covers complex types with custom equality functions, custom to-string functions and
// dynamic memory allocation and deallocation for arguments and result types:

using result_t = tuple<int, int*>;
using arg_tuple_t = tuple<float, int*>;

// function to be tested - beware: dynamically allocates memory
result_t fun(float f, int* i) {
    int* j = new int(2 * (*i) + f);
    return result_t(*i, j);
}

// reference function - beware: dynamically allocates memory
result_t reference_fun(float f, int* i) {
    int* j = new int(2 * (*i) + f);
    return result_t(*i, j);
}

// ...

auto arg_creator        = [](unsigned int i) { return arg_tuple_t(rand() % 10 * 0.1f, new int(rand() % 10)); };
auto result_comparator  = [](const result_t& a, const result_t& b) { return get<0>(a) == get<0>(b) && *get<1>(a) == *get<1>(b); };
auto args_to_string     = []( const arg_tuple_t& t) { stringstream s; s << "(" << get<0>(t) << ", " << *get<1>(t) << ")"; return s.str(); };
auto result_to_string   = []( const result_t& r){ stringstream s; s << "(" << get<0>(r) << ", " << *get<1>(r) << ", " << ")"; return s.str(); };
auto arg_deleter        = [](const arg_tuple_t& t) { delete get<1>(t); };
auto result_deleter     = [](const result_t& r) { delete get<1>(r); };

unittest::RandomizedFunctionTest<result_t, float, int*> tester(
    fun,
    reference_fun,
    arg_creator,
    result_comparator,
    args_to_string,
    result_to_string,
    arg_deleter,
    result_deleter);

// ...


// To test a method on an object, I'm afraid you have to wrap the method call into a lambda.



2. TODO ###########################################################################################
###################################################################################################


TODO test-functions for functions with side-effects. class StatusTest;
    if it really pays off
    lookat: http://stackoverflow.com/questions/16868129/how-to-store-variadic-template-arguments

TODO test-functions that test for exceptions. test_exceptions(...)

TODO test functions for speed
    lookat: http://stackoverflow.com/questions/255645/how-can-i-count-operations-in-c

TODO test-function that aggregates all test functions. test_function(...)


TODO implement test_FunctionTest.cpp


3. HISTORY ########################################################################################
###################################################################################################


160205      - added RandomizedFunctionTest for randomized function tests
            - removed helper functions for FunctionTest, added internal structs instead.
            - langenhagen: added a version of my tuple_to_stream for tuple printing.

160126      - added FunctionTest::write_test_series_summary()
            - changed FunctionTest::test() return type and added helper functions.
            - added file test_FunctionTest.cpp

160121      - some fine grained fixes on the FunctionTest class
            - enforced correct result type on FunctionTest::test()


# END OF FILE #
