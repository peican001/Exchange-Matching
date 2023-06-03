#! /bin/bash

TEST_FILE1=test1.xml
TEST_FILE2=test2.xml
TEST_FILE3=test3.xml
TEST_FILE4=test4.xml
TEST_FILE5=test5.xml
TEST_FILE6=test6.xml
TEST_FILE7=test7.xml
TEST_FILE8=test8.xml

# change this number to adjust the request sent
NUM_REQUESTS=5

./test $TEST_FILE1

for ((i = 0; i < $NUM_REQUESTS; ++i))
do
	./test $TEST_FILE1 &
	./test $TEST_FILE2 &
	./test $TEST_FILE3 &
	./test $TEST_FILE4 &
	./test $TEST_FILE5 &
	./test $TEST_FILE6 &
	./test $TEST_FILE7 &
	./test $TEST_FILE8 &
done

