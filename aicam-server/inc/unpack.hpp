struct global_args_t {
    MYSQL *mysql;
    MYSQL *ptr_mysql;
    filter_t filter;
    global_args_t() {
    }
};
