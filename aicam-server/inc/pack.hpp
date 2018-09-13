struct pack_t {
    uint32_t total_len;
    uint32_t total_frame;
    uint32_t collect_id;
    uint32_t camera_id[16];
    uint64_t capture_time[16];
    uint32_t frame_size[16];
    uint64_t event_id[16];
    uint8_t collect_type;
    uint8_t is_end[16];
};

struct Pack_frame_version {
  uint32_t frame_version;
};

struct Pack_time {
  uint32_t second;
  uint32_t nanosecond;
};

struct Pack_camera_id {
  uint32_t camera_id;
};

struct Pack_image {
  char image_url[256];
};

struct Pack_goods_model {
  char goods_model_url[256];
};

struct Pack_face_model {
  char face_model_url[256];
};

struct Pack_gesture_model {
  char gesture_model_url[256];
};

struct Pack_body_model {
  char body_model_url[256];
};

struct Pack_image_flag {
  uint32_t image_flag;
};

struct Pack_event_id {
  uint64_t event_id;
};

struct Pack_diff_position {
  char diff_position_url[256];
};

struct Pack_depth {
  char depth_url[256];
};

struct Pack_right {
  char right_url[256];
};
