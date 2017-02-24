#!/bin/bash
# Bash script to test question 3 of homework 1
# Usage :- chmod +x test.sh
#          ./test.sh
#
#sh script to test question 3 of homework 1
# Usage :- chmod +x test-1.sh
#          ./test-1.sh

# run make and make clean and also create a new disk image

make clean > /dev/null
rm *.gcno *.gcda
make COVERAGE=1 > /dev/null
rm -rf foo.img
./mktest foo.img

#-------------------------------------------------------------------
# TEST CASE# 1 : To check that basic ls and ls-l works i.e. command 
#                does not fail. 
#-------------------------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
ls
ls-l
ls file.A
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.1 FAILED; exit; }
read/write block size: 1000
cmd> ls
dir1
file.7
file.A
cmd> ls-l
dir1 drwxr-xr-x 0 0 Fri Jul 13 07:08:00 2012
file.7 -rwxrwxrwx 6644 7 Fri Jul 13 07:06:20 2012
file.A -rwxrwxrwx 1000 1 Fri Jul 13 07:04:40 2012
cmd> ls file.A
error: Not a directory
cmd> quit
EOF
if [ $? -eq 0 ]; then
    echo "Test q1.1 passed"
else
    echo "Test q1.1 failed"
fi
#---------------------------------------------------------------------
# TEST CASE# 2 : To test whether ls-l works for all the negative cases
#---------------------------------------------------------------------

./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
ls dir2
ls-l /dir1/file.270/
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.2 FAILED; exit; }
read/write block size: 1000
cmd> ls dir2
error: No such file or directory
cmd> ls-l /dir1/file.270/
/dir1/file.270/ -rwxrwxrwx 276177 270 Fri Jul 13 07:06:20 2012
cmd> quit
EOF
if [ $? -eq 0 ]; then
    echo "Test q1.2 passed"
else
    echo "Test q1.2 failed"
fi
#---------------------------------------------------------------------
# TEST CASE# 3 : To test whether mkdir works for positive cases
#---------------------------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
mkdir dir2
ls
mkdir dir2/dir3
cd dir2
ls
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.3 FAILED; exit; }
read/write block size: 1000
cmd> mkdir dir2
cmd> ls
dir1
dir2
file.7
file.A
cmd> mkdir dir2/dir3
cmd> cd dir2
cmd> ls
dir3
cmd> quit
EOF
if [ $? -eq 0 ]; then
    echo "Test q1.3 passed"
else
    echo "Test q1.3 failed"
fi

#------------------------------------------------
# TEST CASE# 4 : To test mkdir for error cases
#------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
mkdir dir2
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.4 FAILED; exit; }
read/write block size: 1000
cmd> mkdir dir2
error: File exists
cmd> quit
EOF
if [ $? -eq 0 ]; then
    echo "Test q1.4 passed"
else
    echo "Test q1.4 failed"
fi
#------------------------------------------------
# TEST CASE# 5 : To test rmdir for error cases
#------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
rmdir dir1
rmdir dir3
rmdir file.A
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.5 FAILED; exit; }
read/write block size: 1000
cmd> rmdir dir1
error: Directory not empty
cmd> rmdir dir3
error: No such file or directory
cmd> rmdir file.A
error: Not a directory
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.5 passed"
else
    echo "Test q1.5 failed"
fi
#------------------------------------------------
# TEST CASE# 6 : To test rmdir for positive cases
#------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
rmdir dir2/dir3
rmdir dir2
ls
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.6 FAILED; exit; }
read/write block size: 1000
cmd> rmdir dir2/dir3
cmd> rmdir dir2
cmd> ls
dir1
file.7
file.A
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.6 passed"
else
    echo "Test q1.6 failed"
fi
#------------------------------------------------
# TEST CASE# 7 : To test chmod for positive cases
#                and negative cases
#------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
chmod 755 dir1
chmod 777 file.7
chmod 777 file.88
ls-l
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.7 FAILED; exit; }
read/write block size: 1000
cmd> chmod 755 dir1
cmd> chmod 777 file.7
cmd> chmod 777 file.88
error: No such file or directory
cmd> ls-l
dir1 drwxr-xr-x 0 0 Fri Jul 13 07:08:00 2012
file.7 -rwxrwxrwx 6644 7 Fri Jul 13 07:06:20 2012
file.A -rwxrwxrwx 1000 1 Fri Jul 13 07:04:40 2012
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.7 passed"
else
    echo "Test q1.7 failed"
fi
#------------------------------------------------
# TEST CASE# 8 : To test rename for error cases
#------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
rename file.A file.7
rename file.23 file.27
rename /dir1/file.270 file.9
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.7 FAILED; exit; }
read/write block size: 1000
cmd> rename file.A file.7
error: File exists
cmd> rename file.23 file.27
error: No such file or directory
cmd> rename /dir1/file.270 file.9
error: Invalid argument
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.8 passed"
else
    echo "Test q1.8 failed"
fi
#------------------------------------------------
# TEST CASE# 9 : To test rename for positive cases
#------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
rename dir1 dir2
rename file.7 file.7
rename file.A file.B
ls
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.9 FAILED; exit; }
read/write block size: 1000
cmd> rename dir1 dir2
cmd> rename file.7 file.7
cmd> rename file.A file.B
cmd> ls
dir2
file.7
file.B
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.9 passed"
else
    echo "Test q1.9 failed"
fi
#-------------------------------------------------------------------
# TEST CASE# 10 : To check unlink and truncate for negative cases
#-------------------------------------------------------------------
rm -rf foo.img
./mktest foo.img

./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
rm file.270
rm dir1
rm /dir2/file.0
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.10 FAILED; exit; }
read/write block size: 1000
cmd> rm file.270
error: No such file or directory
cmd> rm dir1
error: Is a directory
cmd> rm /dir2/file.0
error: No such file or directory
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.10 passed"
else
    echo "Test q1.10 failed"
fi
#-------------------------------------------------------------------
# TEST CASE# 11 : To check unlink and truncate for positive cases
#-------------------------------------------------------------------
rm -rf foo.img
./mktest foo.img

./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
rm /dir1/file.270
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.11 FAILED; exit; }
read/write block size: 1000
cmd> rm /dir1/file.270
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.11 passed"
else
    echo "Test q1.11 failed"
fi
#-------------------------------------------------------------------
# TEST CASE# 12 : To test statfs
#-------------------------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
statfs
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.12 FAILED; exit; }
read/write block size: 1000
cmd> statfs
max name length: 27
block size: 1024
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.12 passed"
else
    echo "Test q1.12 failed"
fi
#---------------------------------------------------
# TEST CASE# 13 : To test truncate for negative case
#---------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
truncate /dir1
truncate /dir2/file.A
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.13 FAILED; exit; }
read/write block size: 1000
cmd> truncate /dir1
error: Is a directory
cmd> truncate /dir2/file.A
error: No such file or directory
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.13 passed"
else
    echo "Test q1.13 failed"
fi
#---------------------------------------------------
# TEST CASE# 14 : To test truncate for positive cases
#---------------------------------------------------
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
truncate file.7
truncate file.A
truncate /dir1/file.270
ls
cd dir1
ls
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q1.14 FAILED; exit; }
read/write block size: 1000
cmd> truncate file.7
cmd> truncate file.A
cmd> truncate /dir1/file.270
error: No such file or directory
cmd> ls
dir1
file.7
file.A
cmd> cd dir1
cmd> ls
file.0
file.2
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q1.14 passed"
else
    echo "Test q1.14 failed"
fi

#-------------------------------------------------------------------
# TEST CASE# 15 : Addition tests for remove different size file
#-------------------------------------------------------------------
rm -rf foo.img
./mktest foo.img
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
cd dir1
rm file.0
quit
EOF

test $(./read-img foo.img | tail -2 | wc --words) -eq 4 && echo "Test q1.15.1 passed"|| echo "Test q1.15 failed"


rm -rf foo.img
./mktest foo.img
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
cd dir1
rm file.2
quit
EOF

test $(./read-img foo.img | tail -2 | wc --words) -eq 4 && echo "Test q1.15.2 passed"|| echo "Test q1.15 failed"

rm -rf foo.img
./mktest foo.img
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
rm file.7
quit
EOF

test $(./read-img foo.img | tail -2 | wc --words) -eq 4 && echo "Test q1.15.3 passed"|| echo "Test q1.15 failed"

rm -rf foo.img
./mktest foo.img
./homework -cmdline -image foo.img <<EOF > /tmp/test1-output
cd dir1
rm file.270
quit
EOF

test $(./read-img foo.img | tail -2 | wc --words) -eq 4 && echo "Test q1.15.4 passed"|| echo "Test q1.15 failed"

#-------------------------------------------------------------------
# TEST CASE# 1 : To check read and write works as expected includes
#                -ve cases for read
#-------------------------------------------------------------------
rm -rf disk.img
./mktest disk.img

./homework -cmdline -image disk.img <<EOF > /tmp/test1-output
cd dir1
get file.270
cd ..
put file.270
ls
show /dir1
show /dir5/dir6
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q2.1 FAILED; exit; }
read/write block size: 1000
cmd> cd dir1
cmd> get file.270
cmd> cd ..
cmd> put file.270
cmd> ls
dir1
file.270
file.7
file.A
cmd> show /dir1
error: Is a directory
cmd> show /dir5/dir6
error: No such file or directory
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q2.1 passed"
else
    echo "Test q2.1 failed"
fi



#-------------------------------------------------------------------
# TEST CASE# 2 : To test write with different size of files
#-------------------------------------------------------------------
rm -rf disk.img
./mktest disk.img

dd if=/dev/zero of=file.88 bs=88 count=1
dd if=/dev/zero of=file.1024 bs=1024 count=1
dd if=/dev/zero of=file.2000 bs=2000 count=1

./homework -cmdline -image disk.img <<EOF > /tmp/test1-output
put file.88
put file.1024
put file.2000
ls
get file.7
rm file.7
ls
cd dir1
put file.7
ls
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q2.2 FAILED; exit; }
read/write block size: 1000
cmd> put file.88
cmd> put file.1024
cmd> put file.2000
cmd> ls
dir1
file.1024
file.2000
file.7
file.88
file.A
cmd> get file.7
cmd> rm file.7
cmd> ls
dir1
file.1024
file.2000
file.88
file.A
cmd> cd dir1
cmd> put file.7
cmd> ls
file.0
file.2
file.270
file.7
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q2.2 passed"
else
    echo "Test q2.2 failed"
fi

#-------------------------------------------------------------------
# TEST CASE# 3 : To check out of disk space during multiple writes
#-------------------------------------------------------------------
rm -rf disk.img
./mktest disk.img

./homework -cmdline -image disk.img <<EOF > /tmp/test1-output
put file.270 /dir1
put file.270 /dir5/
put file.270 file.2701
put file.270 file.2702
ls
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q2.3 FAILED; exit; }
read/write block size: 1000
cmd> put file.270 /dir1
error: File exists
cmd> put file.270 /dir5/
error: No such file or directory
cmd> put file.270 file.2701
cmd> put file.270 file.2702
cmd> ls
dir1
file.2701
file.2702
file.7
file.A
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q2.3 passed"
else
    echo "Test q2.3 failed"
fi

# creating a new image file
rm -rf disk.img
./mktest disk.img

#-------------------------------------------------------------------
# TEST CASE# 4 : To check for invalid path while writing a file
#-------------------------------------------------------------------
./homework -cmdline -image disk.img <<EOF > /tmp/test1-output
put file.270 /dir5/file.270
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q2.4 FAILED; exit; }
read/write block size: 1000
cmd> put file.270 /dir5/file.270
error: No such file or directory
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q2.4 passed"
else
    echo "Test q2.4 failed"
fi

#-------------------------------------------------------------------
# TEST CASE# 5 : read
#-------------------------------------------------------------------
rm -rf disk.img
./mktest disk.img

./homework -cmdline -image disk.img <<EOF > /tmp/test1-output
show file.A
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q2.5 FAILED; exit; }
read/write block size: 1000
cmd> show file.A
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAcmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q2.5 passed"
else
    echo "Test q2.5 failed"
fi

#-------------------------------------------------------------------
# TEST CASE# 6 : To check out of disk space during multiple writes
#-------------------------------------------------------------------
rm -rf disk.img
./mktest disk.img

./homework -cmdline -image disk.img <<EOF > /tmp/test1-output
put file.270 /dir1
put file.270 /dir5/
put file.270 file.2701
put file.270 file.2702
put file.270 file.2703
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q2.6 FAILED; exit; }
read/write block size: 1000
cmd> put file.270 /dir1
error: File exists
cmd> put file.270 /dir5/
error: No such file or directory
cmd> put file.270 file.2701
cmd> put file.270 file.2702
cmd> put file.270 file.2703
EOF

if [ $? -eq 0 ]; then
    echo "Test q2.6 passed"
else
    echo "Test q2.6 failed"
fi

#-------------------------------------------------------------------
# TEST CASE# 6.1 : Follow up the above test since when no space available 
# in FS, it will break, so we need to check if the last file actually
# get written into the image
#-------------------------------------------------------------------
./homework -cmdline -image disk.img <<EOF > /tmp/test1-output
ls
quit
EOF

cmp /tmp/test1-output <<EOF || { echo TEST q2.6 FAILED; exit; }
read/write block size: 1000
cmd> ls
dir1
file.2701
file.2702
file.2703
file.7
file.A
cmd> quit
EOF

if [ $? -eq 0 ]; then
    echo "Test q2.6.1 passed"
else
    echo "Test q2.6.1 failed"
fi

./mkfs-x6 -size 50m big.img
cat file.270 >> largeFile.270
cat file.270 >> largeFile.270
cat file.270 >> largeFile.270

