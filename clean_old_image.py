
import time
import glob
import os



for i in range(0, 10000000):
    print('round : {}'.format(i))
    cur_time = time.time() * 1000 - 30000
    
    fps = glob.glob('tmp/depth_cam1/color/*.png')
    fps .extend(glob.glob('tmp/depth_cam1/depth/*.png'))
    fps .extend(glob.glob('tmp/depth_cam2/color/*.png'))
    fps .extend(glob.glob('tmp/depth_cam2/depth/*.png'))
    for fp in fps:
        t = int(fp.split('/')[-1].split('.')[0])
        print(t, cur_time)
        if t < cur_time:
            os.remove(fp)
            flag_path = fp[:-3] + 'flag'
            if os.path.exists(flag_path):
                os.remove(flag_path)
    time.sleep(10)

