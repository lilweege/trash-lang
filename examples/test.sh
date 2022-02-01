#!/bin/sh

# https://stackoverflow.com/a/43919044/12637867
a="/$0"; a=${a%/*}; a=${a#/}; a=${a:-.}; BASEDIR=$(cd "$a"; pwd)

cd $BASEDIR/..
make || exit 1

expect() {
    fn=$1
    expect=$2

	$BASEDIR/../bin/trash -c $BASEDIR/$fn.trash >/dev/null && result=$(./$fn.out)
	if [ $? -ne 0 ]; then
		echo "something went wrong"
		exit 1
	fi

	if [ "$result" = "$expect" ]; then
		echo "PASSED $fn"
	else
		echo "FAILED $fn"
		echo "EXPECT: $expect"
		echo "ACTUAL: $result"
		exit 1
	fi
}

# do tests
expect hello "hello world"
expect hell0 "hello w0rld"
expect fact "2432902008176640000"
expect fib "7778742049"
expect collatz "111"
expect pi "3.14159165"
expect prime "78498"

expect rule110 "                  * 
                 ** 
                *** 
               ** * 
              ***** 
             **   * 
            ***  ** 
           ** * *** 
          ******* * 
         **     *** 
        ***    ** * 
       ** *   ***** 
      *****  **   * 
     **   * ***  ** 
    ***  **** * *** 
   ** * **  ***** * 
  ******** **   *** 
 **      ****  ** * "

expect fizz "1
2
Fizz
4
Buzz
Fizz
7
8
Fizz
Buzz
11
Fizz
13
14
FizzBuzz
16
17
Fizz
19
Buzz
Fizz
22
23
Fizz
Buzz
26
Fizz
28
29
FizzBuzz
31
32
Fizz
34
Buzz
Fizz
37
38
Fizz
Buzz
41
Fizz
43
44
FizzBuzz
46
47
Fizz
49
Buzz
Fizz
52
53
Fizz
Buzz
56
Fizz
58
59
FizzBuzz
61
62
Fizz
64
Buzz
Fizz
67
68
Fizz
Buzz
71
Fizz
73
74
FizzBuzz
76
77
Fizz
79
Buzz
Fizz
82
83
Fizz
Buzz
86
Fizz
88
89
FizzBuzz
91
92
Fizz
94
Buzz
Fizz
97
98
Fizz
Buzz"
