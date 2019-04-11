

ostype=$(sh check_os.sh)
if [ X$ostype = Xlinux ]; then 
sudo apt install -y libeigen3-dev
sudo apt install -y libgflags-dev
sudo apt install -y libgoogle-glog-dev
sudo apt install -y libboost-all-dev
fi

cmake .
make -j 8
