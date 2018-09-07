
base_dir=$1
[ X${base_dir} = X ] && echo "base_dir is not set" && exit 1

folder_path=${base_dir}/calib_data
output_folder=${base_dir}/parameters
num_camera=`ls ${folder_path} | wc -l `

[ ! -e ${output_folder} ] && mkdir -p $output_folder

./MultiCamCalib/multicamera_calibration  \
	-folder_path $folder_path  \
	-output_folder $output_folder \
	-num_camera $num_camera \
	# -show_reproj_det \