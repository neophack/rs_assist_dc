


source ./setenv.sh

[ $# -lt 1 ] && echo $0 basedir && exit 1


cd aicam_server
sh start.sh
cd  ..

echo 'start data_collection ...'

basedir=$1
mkdir -p $basedir

python gen_tm.py $*


