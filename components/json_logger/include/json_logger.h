#ifndef JSON_LOGGER
#define JSON_LOGGER


void spiffs_init();
void spiffs_terminate();

void create_json(char file_loc[]);
void create_json_settings(char file_loc[]);
void log_data_array(char file_loc[], char key[], char value[], int max_length);
void log_data(char file_loc[], char key[], char value[], int max_length);
void get_data_array(char file_loc[], char key[], float data_array[]);
void set_data_array(char file_loc[], char key[], char value[], int max_length, int position);
void print_logged_data(char file_loc[]);

#endif
