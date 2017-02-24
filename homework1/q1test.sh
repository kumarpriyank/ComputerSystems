echo This is q1prog test

./homework q1 > tmp.out
 echo Hello world > ref.out
{ diff -u --label='your code' --label='reference' tmp.out ref.out; echo q1.1 PASS;} \
|| { echo TEST q1.1 FAILED; exit; }

 valgrind ./homework q1 >> tmp.out 2<&1
 grep "ERROR SUMMARY" tmp.out
