
# this build script is tested only on ubuntu 16.04
echo 'build shel-cam-3d'
cd shel-cam-3d && sh build.sh && cd ..

echo 'build MultiCamCalib'
cd MultiCamCalib && sh build.sh && cd ..

echo 'build aicam-server'
cd aicam-server && sh build.sh && cd ..
[ ! -e aicam_server ] && ln -s aicam-server aicam_server

echo 'build build_pyrealsense'
sh build_pyrealsense.sh


