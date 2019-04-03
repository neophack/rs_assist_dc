


source ./setenv.sh

cd aicam-server
sh start.sh
cd  ..

rm -rf depth_cam*
ps -ef|grep rs2_multi_align.py |awk '{print $2}'|xargs kill -9
nohup python rs2_multi_align.py > rs2_multi_align.log 2>&1 &

echo 'sleep 2'
sleep 2


echo 'start data_collection ...'

basedir='data/batch5'
mkdir -p $basedir

python gen_tm.py $basedir


