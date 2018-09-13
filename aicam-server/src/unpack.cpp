#include <memory.h>
#include <unistd.h>
#include "tcp_server.hpp"
#include "pack.hpp"
#include "unpack.hpp"
const std::string WEB_HOST="192.168.0.1:8200";
//const std::string WEB_ROOT="/home/xteam/newsale/orp/webroot";
//const std::string WEB_ROOT="/home/xteam/space/aicam-server/data";
const std::string WEB_ROOT="data";
extern Table_type table_type;
//int id = 0;
int mysql_insert(std::string sql, MYSQL *mysql);

int unpack(char *buf, int pack_len, void *rec_args) {
    global_args_t *args = (global_args_t *) rec_args;
    //printf("111111111111111 Total length: %d\n", pack_len);
    Pack_frame_version pack_frame_version;
    memcpy(&pack_frame_version, &buf[4], 4*sizeof(char));
    //printf("FRAME VERSION: %u.\n", pack_frame_version.frame_version);
    int index = 8;

    Pack_time pack_time;
    Pack_camera_id pack_camera_id;
    Pack_image pack_image;
    Pack_goods_model pack_goods_model;
    Pack_face_model pack_face_model;
    Pack_gesture_model pack_gesture_model;
    Pack_body_model pack_body_model;
    Pack_image_flag pack_image_flag;
    Pack_event_id pack_event_id;
    Pack_diff_position pack_diff_position;
    Pack_depth pack_depth;
    Pack_right pack_right;
    sprintf(pack_right.right_url, "%s", "");
    uint64_t capture_time_mic;
    char date[14+1];
    char path[128];
    char cmd[256];
    char sub_path[128];

    while(index < pack_len + 4) {
      uint32_t tmp = 0;
      memcpy(&tmp, &buf[index], 4*sizeof(char));
      int obj_type = tmp >> 24;
      int obj_len = tmp & 0x00FFFFFF;
      index += 4;
      switch (obj_type) {
        case 0: {//time
                memcpy(&pack_time, &buf[index], sizeof(pack_time));
                //printf("Message type: TIME. Content: %u : %u\n", pack_time.second, pack_time.nanosecond);
                
                capture_time_mic = static_cast<uint64_t>(pack_time.second) * 1000 + pack_time.nanosecond/1000000;
                time_t tmp_time = pack_time.second;
                struct tm *local_time = localtime(&tmp_time);
                uint32_t year = local_time->tm_year + 1900;
                uint32_t mon = local_time->tm_mon +1;
                uint32_t day = local_time->tm_mday;
                memset(date, 0, sizeof(date));
                //printf("year:%d mon:%2d day:%2d\n", year, mon, day);
                strftime(date, sizeof(date), "%Y%m%d", local_time);
                index += obj_len;
                break;}
        case 1: {//cameraID
                memcpy(&pack_camera_id, &buf[index], sizeof(pack_camera_id));
                memset(path, 0, sizeof(path));
                memset(cmd, 0, sizeof(path));
                memset(sub_path, 0, sizeof(sub_path));
                sprintf(sub_path,"newsale/data/%s/camera-%d", date, pack_camera_id.camera_id);
                sprintf(path,"%s/%s", WEB_ROOT.c_str(),  sub_path);
                sprintf(cmd, "mkdir -p %s", path);
                system(cmd);
                //printf("Message type: CAMERA_ID. Content: %u path:%s\n", pack_camera_id.camera_id,path);
                index += obj_len;
                break;}
        case 2: {//image
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.jpg", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                FILE * fp = fopen(file.c_str(), "w");
                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                printf("Message type: IMAGE. Content saved to %d %s/%s\n", pack_camera_id.camera_id,path,file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_image.image_url, "%s", url.c_str());
                index += obj_len;

                /*memcpy(pack_image.image, &buf[index], obj_len * sizeof(char));
                //id++;
                char filename[20];
                sprintf(filename, "/tmp/%d.jpeg", id);
                FILE * fp = fopen(filename, "w");
                fwrite(pack_image.image, obj_len * sizeof(char), 1,fp);
                //printf("Message type: IMAGE. Content saved to /tmp/%d.jpeg", id);
                index += obj_len;
                free(pack_image.image);*/
                break;}
        case 3: {//goods model
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.goods_model", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                FILE * fp = fopen(file.c_str(), "w");
                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                //printf("Message type: GOODS_MODEL. Content saved to %s", file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_goods_model.goods_model_url, "%s", url.c_str());
                index += obj_len;

                /*pack_goods_model.goods_model = (char *) malloc(obj_len * sizeof(char));
                if (!pack_goods_model.goods_model) {
                  printf("Error happened when malloc goods_model.\n");
                }
                memcpy(pack_goods_model.goods_model, &buf[index], obj_len * sizeof(char));
                //printf("33333333333333333333 length: %d", obj_len);
                printf("Message type: GOODS_MODEL. Content: %02x %02x %02x %02x", pack_goods_model.goods_model[0], pack_goods_model.goods_model[1], pack_goods_model.goods_model[10], pack_goods_model.goods_model[11]);
                index += obj_len;
                free(pack_goods_model.goods_model);*/
                break;}
        case 4: {//face model
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.face_model", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                FILE * fp = fopen(file.c_str(), "w");
                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                //printf("Message type: FACE_MODEL. Content saved to %s", file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_face_model.face_model_url, "%s", url.c_str());
                index += obj_len;
                /*pack_face_model.face_model = (char *) malloc(obj_len * sizeof(char));
                if (!pack_face_model.face_model) {
                  printf("Error happened when malloc face_model.\n");
                }
                memcpy(pack_face_model.face_model, &buf[index], obj_len * sizeof(char));
                //printf("33333333333333333333 length: %d", obj_len);
                //printf("Message type: FACE_MODEL. Content: %02x %02x %02x %02x", pack_face_model.face_model[0], pack_face_model.face_model[1], pack_face_model.face_model[10], pack_face_model.face_model[11]);
                index += obj_len;*/
                break;}
        case 5: {//gesture model
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.gesture_model", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                FILE * fp = fopen(file.c_str(), "w");
                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                //printf("Message type: GESTURE_MODEL. Content saved to %s", file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_gesture_model.gesture_model_url, "%s", url.c_str());
                index += obj_len;

                /*pack_gesture_model.gesture_model = (char *) malloc(obj_len * sizeof(char));
                if (!pack_gesture_model.gesture_model) {
                  printf("Error happened when malloc gesture_model.\n");
                }
                memcpy(pack_gesture_model.gesture_model, &buf[index], obj_len * sizeof(char));
                //printf("33333333333333333333 length: %d", obj_len);
                printf("Message type: GESTURE_MODEL. Content: %02x %02x %02x %02x", pack_gesture_model.gesture_model[0], pack_gesture_model.gesture_model[1], pack_gesture_model.gesture_model[10], pack_gesture_model.gesture_model[11]);
                index += obj_len;*/
                break;}
        case 6: {//body model
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.body_model", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                FILE * fp = fopen(file.c_str(), "w");
                //printf("Message type: BODY_MODEL. Content: %02x %02x %02x %02x. Length: %d", buf[0], buf[1], buf[10], buf[11], obj_len);

                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                //printf("Message type: BODY_MODEL. Content saved to %s", file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_body_model.body_model_url, "%s", url.c_str());
                index += obj_len;

                /*pack_body_model.body_model = (char *) malloc(obj_len * sizeof(char));
                if (!pack_body_model.body_model) {
                  printf("Error happened when malloc body_model.\n");
                }
                memcpy(pack_body_model.body_model, &buf[index], obj_len * sizeof(char));
                //printf("33333333333333333333 length: %d", obj_len);
                printf("Message type: BODY_MODEL. Content: %02x %02x %02x %02x", pack_body_model.body_model[0], pack_body_model.body_model[1], pack_body_model.body_model[10], pack_body_model.body_model[11]);
                index += obj_len;*/
                break;}
        case 7: {//image flag
                memcpy(&pack_image_flag, &buf[index], sizeof(pack_image_flag));
                //printf("Message type: IMAGE_FLAG. Content: %u\n", pack_image_flag.image_flag);
                index += obj_len;
                break;}
        case 8: {//event id
                memcpy(&pack_event_id, &buf[index], sizeof(pack_event_id));
                //printf("Message type: EVENT_ID. Content: %lu\n", pack_event_id.event_id);
                index += obj_len;
                break;}
        case 9: {//diff position
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.diff_position", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                FILE * fp = fopen(file.c_str(), "w");
                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                //printf("Message type: DIFF_POSITION. Content saved to %s", file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_diff_position.diff_position_url, "%s", url.c_str());
                index += obj_len;

                /*pack_diff_position.diff_position = (char *) malloc(obj_len * sizeof(char));
                if (!pack_diff_position.diff_position) {
                  printf("Error happened when malloc diff_position.\n");
                }
                memcpy(pack_diff_position.diff_position, &buf[index], obj_len * sizeof(char));
                //printf("33333333333333333333 length: %d", obj_len);
                printf("Message type: DIFF_POSITION. Content: %02x %02x %02x %02x", pack_diff_position.diff_position[0], pack_diff_position.diff_position[1], pack_diff_position.diff_position[10], pack_diff_position.diff_position[11]);
                index += obj_len;
                free(pack_diff_position.diff_position);*/
                break;}
        case 10: {//depth
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.depth", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                //printf("Message type: DEPTH. Content: %02x %02x %02x %02x", buf[index+0], buf[index+1], buf[index+10], buf[index+11]);
                FILE * fp = fopen(file.c_str(), "w");
                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                //printf("Message type: DEPTH. Content saved to %s", file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_depth.depth_url, "%s", url.c_str());
                index += obj_len;

                break;}
        case 11: {//right
                char file_name[128];
                memset(file_name, 0, sizeof(file_name));
                sprintf(file_name, "%lu.right", capture_time_mic);
                std::string file = "";
                file.append(path);
                file.append("/");
                file.append(file_name);
                //printf("Message type: RIGHT. Content: %02x %02x %02x %02x", buf[index+0], buf[index+1], buf[index+10], buf[index+11]);
                FILE * fp = fopen(file.c_str(), "w");
                fwrite(&buf[index],obj_len, 1,fp);
                uint64_t write_time = time(0);
                //printf("write_time:%lu\n",write_time);
                //printf("Message type: RIGHT. Content saved to %s", file_name);
                fclose(fp);
                std::string url = "http://" + WEB_HOST + "/" ;
                url.append(sub_path);
                url.append("/");
                url.append(file_name);
                sprintf(pack_right.right_url, "%s", url.c_str());
                index += obj_len;
                break;}
        default:
                {printf("Error happened. Unknown case %d\n", obj_type);
                break;}
      }
    }
    //printf("--------------------------------->: index: %d, total_len: %d\n", index , pack_len);
    //insert into database
    //
/*
    switch (table_type) {
      case goods: {
          //printf("========================%d\n",pack_image_flag.image_flag);
          if (pack_image_flag.image_flag == 1) {
            char sql[1024];
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "insert into t_frame_goods_aicam (camera_id, capture_time_mic, create_time, image_url, goods_model_url, event_id, image_flag) values (%d, %lu, now(), '%s', '%s', %lu, %u)", pack_camera_id.camera_id, capture_time_mic, pack_image.image_url, pack_goods_model.goods_model_url, pack_event_id.event_id, pack_image_flag.image_flag);
            mysql_insert(sql,args->mysql);
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "insert into t_event_goods_aicam (capture_time_mic_start, event_id, processed) values (%lu, %lu, '0')", capture_time_mic, pack_event_id.event_id);
            mysql_insert(sql,args->mysql);
          } else if (pack_image_flag.image_flag == 2) {
            char sql[1024];
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "insert into t_frame_goods_aicam (camera_id, capture_time_mic, create_time, image_url, goods_model_url, event_id, image_flag) values (%d, %lu, now(), '%s', '%s', %lu, %u)", pack_camera_id.camera_id, capture_time_mic, pack_image.image_url, pack_goods_model.goods_model_url, pack_event_id.event_id, pack_image_flag.image_flag);
            mysql_insert(sql,args->mysql);
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "UPDATE `t_event_goods_aicam` SET `capture_time_mic_end` = %lu, `processed` = '1', `diff_position_url` = '%s'  WHERE `event_id` = %lu", capture_time_mic, pack_diff_position.diff_position_url, pack_event_id.event_id);
            mysql_insert(sql,args->mysql);
          } else if (pack_image_flag.image_flag == 0) {
            char sql[1024];
            memset(sql, 0, sizeof(sql));
            sprintf(sql, "insert into t_frame_goods_aicam (camera_id, capture_time_mic, create_time, image_url, event_id, image_flag) values (%d, %lu, now(), '%s', %lu, %u)", pack_camera_id.camera_id, capture_time_mic, pack_image.image_url, pack_event_id.event_id, pack_image_flag.image_flag);
            mysql_insert(sql,args->mysql);
          }
          break;
      }
      case human: {
        char sql[1024];
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "insert into t_frame_human_aicam (camera_id, capture_time_mic, create_time, image_url, human_url, depth_url, right_url) values (%d, %lu, now(), '%s', '%s', '%s', '%s')", pack_camera_id.camera_id, capture_time_mic, pack_image.image_url, pack_body_model.body_model_url, pack_depth.depth_url, pack_right.right_url);
        mysql_insert(sql,args->mysql);
        break;
      }
      default: {
        printf("Error happened. Unknown case %d\n", table_type);
        break;
      }
    }
    */
    return 0;
}



int mysql_insert(std::string sql,MYSQL *mysql) {
    int res = mysql_query(mysql, sql.c_str());
    printf("insert mysql %d\n",res);
    if(res != 0) {
	int err_no = mysql_errno(mysql);
	const char *err_msg = mysql_error(mysql);
	printf("mysql errno:%d,errmsg:%s\n", err_no, err_msg);
    }
    //printf("res:%d,sql:%s\n", res, sql.c_str());
    return 0;
}
void * init_rec_args(void *rec_args) {
        global_args_t * args = new global_args_t();
        args->mysql = new MYSQL();
        mysql_init(args->mysql);
        MYSQL *ptr_mysql = mysql_real_connect(args->mysql, "192.168.0.1", "work", "123456", "newsale", 3306, NULL, 0);
        if(ptr_mysql) {
                printf("Connection success\n");
        }else {
                printf("Connection failed\n");
        }
        return (void *) args;
}

void destroy_thread(void *args) {
        global_args_t *args_tmp = (global_args_t *) args;
        mysql_close(args_tmp->mysql);
}
