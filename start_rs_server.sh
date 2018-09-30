


source ./setenv.sh


sh stop_rs_server.sh
mkdir -p log
nohup python rs2_multi_align.py > log/rs2_multi_align.log 2>&1 &

echo 'sleep 2'
sleep 2


echo 'desplay start log...'
tail -n 30 log/rs2_multi_align.log



