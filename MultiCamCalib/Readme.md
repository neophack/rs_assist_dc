## 使用说明

可以同时对多个单目摄像头,或者双目摄像头标定外参内参. 现在有的功能是
- 多个单目摄像头内参外参同时标定
- 多个双目摄像头内参外参同时标定
- 多个单目摄像头外参同时标定
- 多个双目摄像头外参同时标定

注意在没有提供内参的情况下,在有某些解不好的情况下,请再多拍几组不同视角下的图片(可以立起来,斜着拍),避免奇异性.

##### 内参说明

标定相机之间的外参时候, 优化只对相机扭曲模型的k1,k2,p1,p2四个参数进行优化, 得到的结果也只是这四个参数, 
要是内参是已经标定好的,你的标定程序应该也只对这四个参数进行标定, 不需要标定8个参数. 可以使用Boteye的矫正程序来标定这4个参数

```bash
./pc_apps/cam_calibration/cam_calibration -folder_path /tmp/13/ -square_size 0.033 -save_calib_yaml /tmp/13/calib13.yaml -show_reproj -single_cam_mode -dist_order 4
```

## 编译

依赖glog, gflags, opencv, yaml

```bash
mkdir build
cd build
cmake ..
make -j2
# 在运行前,请先看main.cpp中那些参数,选择对应的设备,内参文件位置,标定板类型
./multicamera_calibration
```

### 参数

- has_intrinsic
事先已经标定摄像头了吗,已经标定内参true,否则false.

- square_size
标定板一个格子的大小, 0.227是大标定板3*5格子,用于天花板人体追踪的摄像头,0.033是小标定板5*7的格子,用于货架摄像头的标定

- small_board
0.033用于货架摄像头的标定为true, 0.227用于人体追踪的摄像头为false

- stereo
单目摄像头为false, 双目为true

- num_camera
摄像头的个数

### 标定图像的存放位置

图像存储的格式如下: 0,1,2,3,4,5,6是相机的标号,很重要.有多少个相机就建立几个目录.是从0开始的,同个相机在不同时刻采集到的图像要放到该相机目录的文件夹下.

- 单目相机的图像文件格式:
0/cam0 1/cam0/ 2/cam0/ 3/cam0/ 4/cam0 5/cam0 6/cam0

- 双目相机的图像文件格式:(cam0存放的是left image, cam1存放的是right image)
0/cam0 0/cam1 1/cam0 1/cam1 2/cam0 2/cam1

### 标定的好坏

运行最后会输出一些结果, 首先看一下`before optimziation, reproj error`, error基本不会超过2, 
`After optimziation, reproj error`的error基本也不会超过2.

最主要的是看camera的位置,这个可以手量一下,看标定的结果跟手量的是不是基本一致,那说明标定结果不错, 也可以通过看标定板的位置,来说明标定的好坏,
最简单的是看重投影误差,使用下面两个参数来看.

对优化前,检测到的特征点好坏使用参数`-show_det`来看,对优化后的,解的好坏使用`-show_reproj_det`来看重投影误差大小.

最终的内参外参会保存在对应文件里,看命令行的提示.