#pragma once

#define Assert(condition, message) if (!condition) { exit(....); }
#define AssertEquals(expected, actual, message)
#define AssertNotEquals(expected, actual, message)


#define TEST(test_function)

int test_function() {
    // if the assert fails
    return -1;
}

int other_test_function() {

    if (test_function() < 0)
        return -1;
    
}