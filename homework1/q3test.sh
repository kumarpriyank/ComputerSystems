echo This is q3prog test

{ ./homework q3 > tmp.out; echo TEST q3.1 PASS; } || { echo TEST q3.1 FAILED; exit; }

diff -u --label='your output' --label='expected output' tmp.out - <<EOF || { echo TEST q3.2 FAILED; exit; }
program 1
program 2
program 1
program 2
program 1
program 2
program 1
program 2
EOF
echo TEST q3.2 PASS


