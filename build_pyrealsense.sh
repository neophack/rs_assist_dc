if [ $# -ne 1 ]; then
   # echo  "Usage sh $0 PYTHON_EXECUTABLE_PATH" && exit 1
   PYTHON_EXECUTABLE_PATH=`which python`
else
	PYTHON_EXECUTABLE_PATH=$1
fi

sudo apt-get update
sudo apt-get install -y build-essential
sudo apt-get install -y libgtk-3-dev
sudo apt-get install -y pkg-config
sudo apt-get install -y python-dev
sudo apt-get install -y libusb-1.0.0-dev 
sudo apt-get install -y libglfw3-dev
sudo apt-get install -y libgl1-mesa-dev
sudo apt-get install -y libglu1-mesa-dev
#git clone https://github.com/IntelRealSense/librealsense.git
cd librealsense
rm -rf build && mkdir -p build
cd build
cmake ../ -DBUILD_PYTHON_BINDINGS=bool:true  -DPYTHON_EXECUTABLE=$PYTHON_EXECUTABLE_PATH
make -j 8
