


source ./setenv.sh
basedir='data/batch4'



cd aicam_server
sh start.sh
cd  ..

echo 'start data_collection ...'


mkdir -p $basedir

python gen_tm.py $basedir


