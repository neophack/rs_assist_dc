
export PYTHONPATH=`readlink -f librealsense/build/wrappers/python`:$PYTHONPATH
function massacre()
{
   if [ $# -ne 1 ]; then
       echo "usage: massacre relative_tag" 1>&2
       return
   fi
   linenum=`ps aux | grep $1 | grep -v grep | wc -l`

   if [ $linenum -eq 0 ]; then
      echo "No relative process found!" 1>&2
      return
   fi
   for pid in `ps aux | grep $1 | grep -v grep | awk '{print $2}'`;
   do
       echo 'killing pid:'$pid
       kill -9 $pid
   done
}
