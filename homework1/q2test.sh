echo This is q2prog test

./homework q2 > tmp.out <<EOF
q2prog test
test
-- test
h

quit
EOF
{ test -s tmp.out; echo TEST q2.1 PASS; } \
|| { echo TEST q2.1 FAILED; exit; }

./homework q2 > tmp.out <<EOF

q2prog test
test
-- test
h

quit
EOF
{ test -s tmp.out; echo TEST q2.2 PASS; } \
|| { echo TEST q2.2 FAILED; exit; }

mv q2prog q2proggg
./homework q2 > tmp.out <<EOF
q2proggg test
test
-- test

quit
EOF
{ test -s tmp.out; echo TEST q2.3 PASS; } \
|| { echo TEST q2.3 FAILED; exit; }
mv q2proggg q2prog

./homework q2 > tmp.out <<EOF || { echo TEST q2.4 FAILED; exit; }
this-is-not-a-command foo bar
quit
EOF
echo TEST q2.4 PASS

valgrind ./homework q2 > tmp.out 2<&1 <<EOF
q2prog pattern
line without pat...tern
line with pattern

quit
EOF
grep "ERROR SUMMARY" tmp.out
