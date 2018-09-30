


source ./setenv.sh


bash stop_rs_server.sh
echo 'sleep 2'
sleep 2
rm -rf tmp
mkdir -p log
mkdir -p tmp
nohup python rs2_multi_align.py > log/rs2_multi_align.log 2>&1 &
nohup python gbackend/flask_simple_api.py > log/simple_api.log 2>&1 &

echo 'sleep 2'
sleep 2


echo 'desplay start log...'
tail -n 30 log/rs2_multi_align.log
tail -n 30 log/simple_api.log

python gbackend/get_cam.py



