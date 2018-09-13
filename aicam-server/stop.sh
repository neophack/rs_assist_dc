#export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/work/.jumbo/opt/gcc46/lib64:./local/mysql/lib/mysql
#nohup ./collect_goods_server 9255 &
ps -ef|grep collect_goods_server |awk '{print $2}'|xargs kill -9 
