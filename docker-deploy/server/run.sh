# #!/bin/bash
# #sudo su - postgres & psql & CREATE DATABASE MATCH_ENGINE
# make clean
# make all
# echo 'start running server...'
# ./server
# while true ; do continue ; done
#!/bin/bash

make clean

make

ls -al

./server &

while true
do
    sleep 1
done