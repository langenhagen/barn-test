/******************************************************************************
/* @file Contains class FunctionTest for simple and small-scale unit testing.
/*
/* XXX Code of the test function is still messy and could be more readable.
/*
/*
/* Write test series as follows:
###################################################################################################

using namespace unittest;

// function to be tested
std::vector<int> fun(int i, int j) {
    return {1, i, j};
}

// comparison function to check actual and expected result
auto comp = [](const std::vector<int>& a, const std::vector<int>& b){ return std::equal(a.begin(), a.end(), b.begin(), b.end()); };
auto tostring = [](const std::vector<int>& v) { return to_string(v, ", "); };

FunctionTest<std::vector<int>, int, int> tester(fun, comp, tostring);

tester.test("Run 1", { 1,13,15 }, 13, 15);
tester.test("Run 2", { 1,13,15 }, 13, 99);
tester.write_test_series_summary();

###################################################################################################
/*
/*
/* To test a method on an object, I'm afraid you have to wrap the method call into a lambda:
###################################################################################################

struct C {
    int foo(int i, int j) { return i + j; }  //< method to be tested 
};

C c;

auto fun = [&obj = c](int i, int j) { return obj.foo(i, j); };

FunctionTest<int, int, int> tester( fun);

// ...

###################################################################################################
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

#include "verbosity.hpp"


///////////////////////////////////////////////////////////////////////////////
// NAMESPACE, CONSTANTS, TYPE DECLARATIONS/IMPLEMENTATIONS and FUNCTIONS


namespace unittest {

    /** FunctionTest unit testing class wich is capable of invoking arbitrary
    functions with a return value and comparing them to anticipated values. 
    Measures the run-time of the function and writes unit test results to a stream.
    For proper functioning, this class relies on copy assignment 
    of the result types and the argument types.
    @tparam ResultType Return type of the given function.
    @tparam ArgTypes Argument types of the given function.
    */
    template< typename ResultType, typename... ArgTypes>
    class FunctionTest {

    public: // types

        using FunctionType              = const std::function<ResultType(ArgTypes...)>;
        using ComparatorFunctionType    = const std::function<bool(const ResultType&, const ResultType&)>;
        using ToStringFunctionType      = const std::function<std::string(const ResultType&)>;
        using DurationType              = std::chrono::microseconds;

    public: // inner classes

        /// The return type of the test() function.
        struct TestReturnType {
            bool is_passed                      = false;                    ///< Indicates whether the test was correctly passed or not.
            ResultType result;                                              ///< Copy of the returned result of the function invocation.
            DurationType invocation_duration    = DurationType(0);          ///< The invocation duration of the function in microseconds.
        };

    private: // vars

        FunctionType fun_;                                                  ///< The function.
        ComparatorFunctionType comp_;                                       ///< The result comparison function.
        ToStringFunctionType to_string_function_;                           ///< The to-string function for ResultType.
        std::ostream& os_;                                                  ///< The output stream.

        unsigned int n_tests_                           = 0;                ///< Number of tests.
        unsigned int n_passed_tests_                    = 0;                ///< Number of passed tests.
        bool is_last_test_passed_                       = true;             ///< indicates whether the last test passed or not.
        DurationType last_invocation_duration_          = DurationType(0);  ///< The duration of the last function invocation.
        DurationType accumulated_invocation_durations_  = DurationType(0);  ///< The accumulated execution time for all function invocations.
        ResultType last_test_result_;                                       ///< Assignment copy result of the last test.

    public: // vars

        verbosity verbosity_level = verbosity::VERBOSE;                     ///< Defines the verbosity of the stream out amount.   
        unsigned int output_line_length = 60;                               ///< The max number of dots that is shown in the printed lines.

    public: // constructors

        /** Constructor for the function tester.
        @param function A function that must return a value.
        @param comparator A comparison function that compares the actual invocation-results
        with given expected results. The comparison function must have the form
        bool(ResultType, ResultType) or similar.
        @param to_string_function The to-string function for the function's return type.
        Defaults to { std::stringstream ss; ss << result; ss.str(); }.
        @param os An ostream to which the output is streamed.
        */
        FunctionTest(
            FunctionType function, 
            ComparatorFunctionType comparator = [](const ResultType& a, const ResultType& b) { return a == b; },
            ToStringFunctionType to_string_function = [](const ResultType& r) { std::stringstream ss; ss << r; return ss.str(); },
            std::ostream& os = std::cout)
            : 
            fun_(function),
            comp_(comparator),
            to_string_function_(to_string_function),
            os_(os)
        {}


    public: // methods

        /** This deleted function prevents callers from handing expected result of wrong type. 
        This prevents false negatives through conversion errors, like double to int truncation. 
        Consider a test on a function int(){ return 1 }:
        tester.test("1", 1.2);    won't compile (otherwise, it would convert to 1 which then yielded OK)
        tester.test("2", 1);      compiles.
        */
        template< typename T>
        TestReturnType test(const std::string&, const T&, const ArgTypes&...) const = delete;


        /** Unit test on the function that is connected to the tester.
        Tests whether the return-value of a given function invoked with given parameters is equal to a given value.
        Also measures the time the function execution takes and writes the results of the test to a given output-stream.
        Checks also for exceptions and reports them to the output stream.
        In case of error the object's flag .verbose in conjunction with a valid result_to_string_function
        can be used to write more sophisticated output.
        @param test_name A human-readable alias of the test that will be written into the stream.
        @param expected_result The anticipated return value of the tested function.
        @param args The arguments that will be passed to the function on invocation.
        @return An object of type FunctionTest<A,B...>::TestReturnType.
        */
        TestReturnType test( const std::string& test_name, const ResultType& expected_result, const ArgTypes&... args) {
            using namespace std::chrono;

            TestReturnType ret;
            ret.is_passed = false;

            std::string output = "FunctionTest: " + test_name + ": ";
            output.resize(output_line_length, '.');
            log(output + " ", verbosity::NORMAL);

            try {
                const auto clock_start = steady_clock::now();
                const ResultType result = fun_(args...);
                const auto dur = duration_cast<DurationType>(steady_clock::now() - clock_start);
                ret.result = result;

                if (comp_(result, expected_result)) {
                    // correct case

                    ++n_passed_tests_;
                    is_last_test_passed_ = true;
                    ret.is_passed = true;

                    std::stringstream ss;
                    ss << "OK (" << dur.count() << " µs)\n";
                    log( ss.str(), verbosity::NORMAL);
                }
                else {
                    // failure case

                    is_last_test_passed_ = false;

                    std::stringstream ss;
                    ss << "FAILED (" << dur.count() << " µs)\n";
                    log( ss.str(), verbosity::NORMAL);
                    ss.str("");
                    ss << 
                        " RESULT:   " << to_string_function_(result) << "\n" <<
                        " EXPECTED: " << to_string_function_(expected_result) << "\n" <<
                        ".\n";
                    log(ss.str(), verbosity::VERBOSE);
                }

                ++n_tests_;
                last_test_result_ = result;
                last_invocation_duration_ = dur;
                accumulated_invocation_durations_ += dur;
                ret.invocation_duration = dur;
            }
            catch (std::exception& ex) {
                std::stringstream ss;
                ss << 
                    "EXCEPTION\n" <<
                    typeid(ex).name() << ":\n" << 
                    ex.what() << "\n";
                log(ss.str(), verbosity::NORMAL);
            }
            catch (...) {
                log("EXCEPTION\nunknown\n", verbosity::NORMAL);
            }

            return ret;
        }

        
        /** After running a series of FunctionTest::test() invocations, this method can be called to write
        summarized information to the output stream.
        @return TRUE if all tests until now are passed or no test has been executed.
                FALSE otherwise.
        */
        bool write_test_series_summary() const {
            const auto is_all_passed = is_all_tests_passed();
            const auto dur = accumulated_invocation_durations().count();

            std::stringstream ss;
            if (is_all_passed)  ss << "+++ TEST SERIES PASSED +++  :)";
            else                ss << "--- SOME TESTS FAILED  ---  :(((";

            ss << 
                "       (" << n_passed_tests() << "/" << n_tests() << ")   (accumulated: " << dur << " µs)\n"
                "\n";

            log(ss.str(),  verbosity::SILENT);

            return is_all_passed;
        }


    public: // getters

        /// Returns the number of tests.
        inline unsigned int n_tests() const { return n_tests_; }

        /// Returns the number of passed tests.
        inline unsigned int n_passed_tests() const { return n_passed_tests_; }

        /// Returns if the last test whas passed. Also returns TRUE if no test was executed.
        inline bool is_last_test_passed() const { retun is_last_test_passed_; }

        /// The duration of the last function invocation.
        inline DurationType last_invocation_duration() const { return last_invocation_duration_; }

        /// Returns a assignment-copy of the result of the last test.
        inline ResultType last_test_result() const { return last_test_result_; }                                

        /// Indicates whether every test so far passed or not. Also returns TRUE if no test was executed.
        inline bool is_all_tests_passed() const { return n_tests_ == n_passed_tests_; }

        /// The accumulated execution time for all function invocations.
        inline DurationType accumulated_invocation_durations() const { return accumulated_invocation_durations_; }
        
    protected: // helpers

        /** Writes the given string to the output stream if the given verbosity level.
        is equal or smaller than the verbosity_level member value.
        @param str The string to be written to a stream;
        @param str_verbosity_level The verbosity level of the given string.
        */
        constexpr void log(const std::string& str, const verbosity str_verbosity_level) const {
            if ( verbosity_level >= str_verbosity_level) {
                os_ << str;
            }
        }

    }; // END class FunctionTest

    
    /** Helper function that creates a FunctionTest object without the need
    to specify the return type of the function.
    It is not better or shorter than the constructors.
    The constructors are to be preferred.
    The function's purpose is to prevent you from re-implementing it.
    @tparam ArgTypes the argument types. auto inferred.
    @tparam the function type. auto inferred.
    @param function The function to be tested.
    @return A FunctionTest object on the given function.
    @param os An ostream to which the output is streamed.
    */
    template< typename... ArgTypes, typename T>
    constexpr auto create_function_test(T function, std::ostream& os = std::cout) {
        return FunctionTest< decltype(function(ArgTypes{}...)), ArgTypes...>(function, os);
    }

} // END namespace unittest