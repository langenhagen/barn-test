/******************************************************************************
/* @file Contains class RandomizedFunctionTest for simple assertion
/*       of function return values from several invocations with
/*       randomly created arguments.
/*
/* XXX The code of the test function is still verry messy and 
/* could be made more readable.
/*
/*
/* A simple example:
###################################################################################################

using namespace unittest;

string fun(float f, int i);             // function to be tested
string reference_fun(float f, int i);   // reference function

// ...

// argument tuple creation function - aways takes an unsigned int as argument
auto arg_creator = 
    [](unsigned int i) { return tuple<float, int>(rand() % 10 *0.1f, rand() % 100); };

RandomizedFunctionTest<string, float, int> tester(fun, reference_fun, arg_creator);

auto test_result = tester.test("Test Run 1", 10000);

// ...

###################################################################################################
/*
/*
/* A complex example of the usage of the RandomizedFunctionTest is shown in the following.
/* It covers complex types with custom equality functions, custom to-string functions and
/* dynamic memory allocation and deallocation for arguments and result types:
###################################################################################################

using result_t    = tuple<int, int*>;
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

RandomizedFunctionTest<result_t, float, int*> tester(
    fun,
    reference_fun,
    arg_creator,
    result_comparator,
    args_to_string,
    result_to_string,
    arg_deleter,
    result_deleter);

// ...

###################################################################################################
/*
/* To test a method on an object, I'm afraid you have to wrap the method call into a lambda.
/*
/*
/* @author langenhagen
/* @version 160206
/*****************************************************************************/
#pragma once

#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>

#include "verbosity.hpp"

///////////////////////////////////////////////////////////////////////////////
// NAMESPACE, CONSTANTS, TYPE DECLARATIONS/IMPLEMENTATIONS and FUNCTIONS


namespace unittest {

    /** RandomizedFunctionTest unit testing class wich is capable of invoking
    arbitrary functions with a return value and comparing them to results of reference functions.
    Measures the run-time of the function and writes unit test results to a stream.
    For proper functioning, this class relies on copy assignment 
    of the result types and the argument types.
    @tparam ResultType Return type of the given function.
    @tparam ArgTypes Argument types of the given function.
    */
    template< typename ResultType, typename... ArgTypes>
    class RandomizedFunctionTest {

    public: // types

        using ArgsTupleType                 = std::tuple<ArgTypes...>;
        using DurationType                  = std::chrono::microseconds;
        using FunctionType                  = const std::function<ResultType(ArgTypes...)>;
        using ComparatorFunctionType        = const std::function<bool(const ResultType&, const ResultType&)>;
        using ArgsCreatorFunctionType       = const std::function<ArgsTupleType(const unsigned int)>;
        using ArgsDeleterFunctionType       = const std::function<void(const ArgsTupleType&)>;
        using ResultDeleterFunctionType     = const std::function<void(const ResultType&)>;
        using ArgsToStringFunctionType      = const std::function<std::string(const ArgsTupleType&)>;
        using ResultToStringFunctionType    = const std::function<std::string(const ResultType&)>;

    public: // inner classes

        /// Stores information on the circumstances which cause a single test to fail.
        struct ErrorCaseType {
            ResultType erroneous_result;    ///< The errorneous function return value.
            ResultType reference_result;    ///< The supposedly correct return value of the reference function.
            ArgsTupleType args;             ///< The corresponding function invocation arguments.
        };

        /// The return type of the RandomizedFunctionTest::test() function.
        struct TestReturnType {
            unsigned int n_tests                            = 0;                ///< # tests.
            unsigned int n_passed_tests                     = 0;                ///< # passed tests.
            DurationType average_invocation_duration        = DurationType(0);  ///< avg function invocation time.
            DurationType accumulated_invocation_durations   = DurationType(0);  ///< accumulated function invocation time.
            std::vector<ErrorCaseType> error_cases;                             ///< Vector of error.

            /// Indicates, wether or not each conducted test was correct or not.
            bool is_all_tests_passed() { return n_tests == n_passed_tests; }
        };

    private: // inner classes

        /// Recursive implementation of the call structure which is used to unpack tuples into function parameters.
        template <typename F, typename Tuple, bool Done, int Total, int... N>
        struct call_impl {

            /// Recursively calls the recursive call_impl::call function until The the last tuple element was unpacked.
            constexpr static auto call(F f, Tuple&& t, DurationType& out_duration) {
                return call_impl<F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t), out_duration);
            }
        };

        /// Final recursive implementation of the call structure.
        template <typename F, typename Tuple, int Total, int... N>
        struct call_impl<F, Tuple, true, Total, N...> {

            /// Final recursive call to the actual function with all arguments unpacked.
            static auto call(F f, Tuple&& t, DurationType& out_duration) {
                using namespace std::chrono;

                const auto clock_start = steady_clock::now();
                auto ret = f(std::get<N>(std::forward<Tuple>(t))...);
                out_duration = duration_cast<DurationType>(steady_clock::now() - clock_start);

                return ret;
            }
        };

    public: // static vars

        static const unsigned int n_function_arguments = sizeof...(ArgTypes);   ///< The number of arguments that the given function and reference function take.

    private: // vars

        FunctionType fun_;                                      ///< The function.
        FunctionType reference_fun_;                            ///< The (correct) reference function.
        ArgsCreatorFunctionType args_creator_;                  ///< Creates and returns a tuple of valid function arguments.
        ComparatorFunctionType comp_;                           ///< The result comparison function. Shall return true when two results are equal.
        ArgsToStringFunctionType args_to_string_function_;      ///< Converts a given function arguments to a string.
        ResultToStringFunctionType result_to_string_function_;  ///< Converts a given function invocation result to a string.
        ArgsDeleterFunctionType args_deleter_;                  ///< A custom deleter function in case some arguments must be manually destoryed.
        ResultDeleterFunctionType result_deleter_;              ///< A custom deleter function in case the function results must be manually destroyed.
        std::ostream& os_;                                      ///< The output stream.

    public: // vars

        verbosity verbosity_level = verbosity::NORMAL;          ///< Defines the verbosity of the stream out amount.
        unsigned int output_line_length = 50;                   ///< The max number of dots that is shown in the printed lines.

    public: // constructors
        
        /** Constructor for the randomized function tester.
        Note that the logic of the class relies on proper copy-assignment
        of the ResultType values and the individual function invocation arguments.
        @param function A function that must return a value.
        The interface must comply with the template-parametrization
        of the object. The function should, given the same arguments,
        should produce the same output as the given reference function.
        @param reference_function A reference function with the same interface as the given function,
        with regard to the parameter list and the return type.
        The reference function should, given the same arguments,
        should produce the same output as the given function.
        @param argument_creator A function of type RandomizedFunctionTest::ArgsTupleType(unsigned int)
        that produces a tuple of function invocation arguments for
        the given function and reference_function. The unsigned int parameter
        can be used to control the argument creation process.
        @param result_comparator A comparison function for the result types. Defaults to '='.
        @param args_to_string_function A to-string function for the argument-tuples.
        Defaults to a standard tuple unpacking and stream-out-created
        string for each tuple element. This may not work for arguments
        for which no stream out operation << can be found.
        @param result_to_string_function A to-string function for the return values
        of function and reference_function.
        Defaults to { std::stringstream ss; ss << result; ss.str(); }.
        This may not work for return types for which no stream out operation << can be found.
        @param argument_deleter Custom deleter for argument tuples. Can be used to free
        memory which has been previously allocated in the given argument_creator,
        e.g. values that are instantiated with new.
        The deleter will be called on each argument tuple for which
        a test has passed, but not on arguments on which the test failed.
        Defaults to a null operation, i.e. { return; }.
        @param result_deleter Custom deleter for the given function and reference_function return values
        Can be used to free memory wich has been previously allocated in the given function and reference_function,
        e.g. values that are instantiated with new. The deleter will be called on each result value
        for which a test has passed, but not on return values on which the test failed.
        Defaults to a null operation, i.e. { return; }.
        @param os An ostream to which the output is streamed. Defaults to std::cout.
        */
        RandomizedFunctionTest(
            FunctionType function,
            FunctionType reference_function,
            ArgsCreatorFunctionType argument_creator,
            ComparatorFunctionType result_comparator = [](const ResultType& a, const ResultType& b) { return a == b; },
            ArgsToStringFunctionType args_to_string_function = [](const ArgsTupleType& t) { std::stringstream ss; tuple_to_stream::to_stream(ss, t); return ss.str(); },
            ResultToStringFunctionType result_to_string_function = [](const ResultType& r) { std::stringstream ss; ss << r; return ss.str(); },
            ArgsDeleterFunctionType argument_deleter = [](const ArgsTupleType&) {},
            ResultDeleterFunctionType result_deleter = [](const ResultType&) {},
            std::ostream& os = std::cout)
            :
            fun_(function),
            reference_fun_(reference_function),
            args_creator_(argument_creator),
            comp_(result_comparator),
            args_to_string_function_(args_to_string_function),
            result_to_string_function_(result_to_string_function),
            args_deleter_(argument_deleter),
            result_deleter_(result_deleter),
            os_(os)
        {}

    public: // methods

        /** Conducts a randomized test series on the function and compares its results
        to the results of the results of the reference function.
        The arguments are created with the argument creator specified in the constructor
        and passed by assignment copy.
        Also measures the time the function execution takes and writes the results of the test to a given output-stream.
        Checks also for exceptions and reports them to the output stream. If an exception occurs,
        the test series will be stopped.
        In case of error the object's flag .verbose in conjunction with a valid result_to_string_function
        can be used to write more sophisticated output.
        @param test_name A human-readable alias of the test that will be written into the stream.
        @param n_tests The number of tests to be conducted.
        @return A RandomizedFunctionTest::TestReturnType object that provides general information about the tests and the error cases.
        */
        TestReturnType test(const std::string& test_name, const unsigned int n_tests) {
            TestReturnType ret;

            std::string output = "RandomizedFunctionTest: " + test_name + ": ";
            const auto dots_total = output_line_length - output.size();
            const float dots_to_add_per_step = static_cast<float>(dots_total) / n_tests;
            float dots_to_add_float = 0;

            log(output, verbosity::NORMAL);

            for (unsigned int i = 0; i < n_tests; ++i) {
                const auto arg_tuple = args_creator_(n_tests);

                dots_to_add_float += dots_to_add_per_step;
                const unsigned int dots_to_add_int = static_cast<unsigned int>(dots_to_add_float);
                log(std::string(dots_to_add_int, '.'), verbosity::NORMAL);
                dots_to_add_float -= dots_to_add_int;

                try {
                    DurationType dur;

                    const auto reference_result = call(reference_fun_, arg_tuple, dur);
                    const auto result = call(fun_, arg_tuple, dur);

                    if (comp_(result, reference_result)) {
                        // correct case
                        ++ret.n_passed_tests;

                        result_deleter_(result);
                        result_deleter_(reference_result);
                    }
                    else {
                        // failure case
                        ErrorCaseType error_case{ result, reference_result, arg_tuple };
                        ret.error_cases.push_back(error_case);
                    }

                    ret.accumulated_invocation_durations += dur;
                }
                catch (std::exception& ex) {
                    std::stringstream ss;
                    ss <<
                        "EXCEPTION\n" <<
                        typeid(ex).name() << ":\n" <<
                        ex.what() << "\n" <<
                        "Arguments: " << args_to_string_function_(arg_tuple) << "\n";
                    log(ss.str(), verbosity::NORMAL);
                    break;
                }
                catch (...) {
                    std::stringstream ss;
                    ss <<
                        "EXCEPTION\n" <<
                        "unknown\n" <<
                        "Arguments: " << args_to_string_function_(arg_tuple) << "\n";
                    log(ss.str(), verbosity::NORMAL);
                    break;
                }

                ++ret.n_tests;
                args_deleter_(arg_tuple);

            } // END for

            if (n_tests > 0) {
                assert(ret.n_tests > 0 && "RandomizedFunctionTest::test().n_test is supposed to be greater 0");
                ret.average_invocation_duration = ret.accumulated_invocation_durations / ret.n_tests;
            }

            const auto avg_dur = ret.average_invocation_duration.count();
            const auto acc_dur = ret.accumulated_invocation_durations.count();

            std::stringstream ss;

            if (n_tests == ret.n_tests && n_tests == ret.n_passed_tests) {
                ss << " OK (";
            }
            else {
                ss << " FAILURE (";
            }

            ss << ret.n_passed_tests << "/" << ret.n_tests << ") (" << avg_dur << " µs avg, " << acc_dur << " µs total)\n";
            log(ss.str(), verbosity::NORMAL);

            unsigned int i = 0;
            for (const auto ec : ret.error_cases) {
                ss.str("");
                ss <<
                    " ERROR CASE " << i++ << ":\n"
                    "   wrong result:        " << result_to_string_function_(ec.erroneous_result) << "\n"
                    "   reference result:    " << result_to_string_function_(ec.reference_result) << "\n"
                    "   args:                " << args_to_string_function_(ec.args) << "\n"
                    " .\n";
                log(ss.str(), verbosity::VERBOSE);
            }

            return ret;
        }

    protected: // helpers

        /** Calls the given function f with the parameters found in the given tuple and,
        if f returns a value, also returns this value.
        @param f A function.
        @param t A tuple containing the parameters for the invocation of f.
        If f takes no arguments, pass std::tuple<>().
        @param[out] out_duration A reference to a duration object to which the execution time
        of the function will be written.
        @return Returns the return value of f.
        If f's result type would be void, the return type of this function would also be void.
        */
        template <typename F, typename Tuple>
        constexpr auto call(F f, Tuple&& t, DurationType& out_duration) const {
            using ttype = std::decay<Tuple>::type;
            return call_impl<F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t), out_duration);
        }


        /** Writes the given string to the output stream if the given verbosity level.
        is equal or smaller than the verbosity_level member value.
        @param str The string to be written to a stream;
        @param str_verbosity_level The verbosity level of the given string.
        */
        constexpr void log(const std::string& str, const verbosity str_verbosity_level) const {
            if (verbosity_level >= str_verbosity_level) {
                os_ << str;
            }
        }

    }; // END class RandomizedFunctionTest

} // END namespace unittest