import sys
import os
import shutil
import time
import json
import datetime
import urllib
import operator
import base64

t_date = (datetime.datetime.now().strftime('%Y%m%d'))
root_path = "aicam_server"
f_path = "%s/data/newsale/data/%s" % (root_path, t_date)
cam_path = "%s/cam_data/newsale/data/%s" % (root_path, t_date)
# deep_path="/home/xteam/space/camdeep"
deep_path = 'tmp'
deep_path_bak = "tmp/bak/%s" % (t_date)
avg_tim = 0

path_cam1_color = deep_path + "/depth_cam1/color/"
path_cam1_deep = deep_path + "/depth_cam1/depth/"
path_cam2_color = deep_path + "/depth_cam2/color/"
path_cam2_deep = deep_path + "/depth_cam2/depth/"

path_cam1_color_bak = deep_path_bak + "/depth_cam1/color/"
path_cam1_deep_bak = deep_path_bak + "/depth_cam1/depth/"
path_cam2_color_bak = deep_path_bak + "/depth_cam2/color/"
path_cam2_deep_bak = deep_path_bak + "/depth_cam2/depth/"

if not os.path.exists(deep_path_bak):
    os.makedirs(deep_path_bak)

if not os.path.exists(path_cam1_color_bak):
    os.makedirs(path_cam1_color_bak)
if not os.path.exists(path_cam1_deep_bak):
    os.makedirs(path_cam1_deep_bak)
if not os.path.exists(path_cam2_color_bak):
    os.makedirs(path_cam2_color_bak)
if not os.path.exists(path_cam2_deep_bak):
    os.makedirs(path_cam2_deep_bak)


def get_deep_file(src_path, dst_path):
    tim_inv = 9999999999999
    now_tim = int(time.time() * 1000)
    src_pic = ""
    src_img_name = ""
    img_path = ""
    for root, dirs, files in os.walk(src_path):
        for f in files:
            if len(f) < 3:
                continue
            fn = long(f.split('.')[0])
            _abs = abs(fn - now_tim)
            if _abs < tim_inv:
                tim_inv = _abs
                if os.path.exists(src_pic):
                    os.remove(src_pic)
                    # print 'remove',src_pic
                src_pic = os.path.realpath("%s/%s" % (root, f))
                src_img_name = f
                # print src_pic,tim_inv
            else:
                os.remove("%s/%s" % (root, f))
                # print 'remove',src_pic
    #shutil.move(src, "%s/%s/" % (cam_path,cam_dir))
    #shutil.copy(src_pic, ("%s/%s/" % (deep_path_bak,"depth_cam1/color/")))
    if len(src_pic) > 2:
        shutil.move(src_pic, dst_path)
        img_path = os.path.realpath("%s/%s" % (dst_path, src_img_name))
        print(img_path,avg_tim)
    else:
        print('not find file :', src_path)
    return img_path


def cap_deep(avg_tim):
    cam1_color_file = get_deep_file(path_cam1_color, path_cam1_color_bak)
    cam1_deep_file = get_deep_file(path_cam1_deep, path_cam1_deep_bak)
    cam2_color_file = get_deep_file(path_cam2_color, path_cam2_color_bak)
    cam2_deep_file = get_deep_file(path_cam2_deep, path_cam2_deep_bak)
    return {"cam1_color_file": cam1_color_file,
            "cam1_dept_file": cam1_deep_file,
            "cam2_color_file": cam2_color_file,
            "cam2_dept_file": cam2_deep_file}


def cap_remote_deep():
    import requests
    target_ip = '127.0.0.1:8950'
    # target_ip = '172.24.24.253:8950'

    resp = requests.get('http://{}/realsense_api/'.format(target_ip)).json()
    tm = str(int(time.time() * 1000))
    d = {}
    for k, v in resp.iteritems():
        if len(v) > 100:
            dst_path = os.path.join('tmp', k + '_' + tm + '.jpg')
            with open(dst_path, 'wb') as fout:
                imgdata = base64.b64decode(v)
                fout.write(imgdata)
        else:
            dst_path = ''
        d[k] = dst_path
    return d


def cap_xeye():
    sum_tim = 0
    t_start = (datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
    img_list = []
    #os.system("cd /home/xteam/newsale/xeye;/bin/bash runxeyecmd.sh startcap;sleep 0.3")
    os.system("cd xeye_client ;sh start.sh >/dev/null;sleep 0.3")
    i = 0
    #print('f_path', f_path)
    for root, dirs, files in os.walk(f_path):
        for cam_dir in dirs:
            last_time = 0
            last_pic = ""
            if not os.path.exists(cam_path + '/' + cam_dir):
                os.makedirs(cam_path + '/' + cam_dir)
            # print 'root1', root
            for root1, dirs1, files1 in os.walk(root + "/" + cam_dir):
                for f in files1:
                    #print root1, f
                    my_time = long(f.split('.')[0])
                    if last_time == 0 or last_time < my_time:
                        last_time = my_time
                        if os.path.exists(last_pic):
                            os.remove(last_pic)
                        last_pic = root + "/" + cam_dir + "/" + f
                    # sum_time=long(f.split('.')[0])
                    # i+=1
                    #cap_file=root+"/"+cam_dir +"/"+f
                    #shutil.move(cap_file, "%s/%s/" % (cam_path,cam_dir))
                    #real_path=os.path.realpath("%s/%s/%s" % (cam_path,cam_dir,f))
                    # img_list.append(real_path)
                    # print real_path
            sum_tim += last_time
            i += 1
            if not os.path.exists(last_pic):
                print('not find last_pic:%s-%s-%s ' % (last_pic,cam_path,cam_dir))
            else:
                shutil.move(last_pic, "%s/%s/" % (cam_path, cam_dir))
                real_path = os.path.realpath("%s/%s/%s.jpg" % (cam_path, cam_dir, last_time))
                img_list.append(real_path)
    avg_tim = -1.0
    if i > 0:
        avg_tim = sum_tim / i
    else:
        avg_tim = sum_tim
        print 'not find xeye images ;'
    t_end = (datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
    print 'start:', t_start, ' end:', t_end, ' num:', len(img_list), avg_tim
    return img_list, avg_tim


def get_image():
    # avg_tim=1535631396551  #1535631396561
    img_list = {}
    deep_img_dict = {}
    img_list, avg_tim = cap_xeye()
    # deep_img_dict = cap_deep(avg_tim)
    deep_img_dict = cap_remote_deep()
    deep_img_dict['xeye_image'] = img_list
    return deep_img_dict


if __name__ == '__main__':
    deep_img_dict = get_image()
    print(json.dumps(deep_img_dict, indent=4))
    # avg_tim=1535631396551  #1535631396561
    # img_list,avg_tim=cap_xeye()
    # deep_img_dict=cap_deep(avg_tim)
    # deep_img_dict["xeye_img_files"]=img_list
    # print deep_img_dict
