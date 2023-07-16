#include "frozen.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_vfs.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "json_logger";

static xSemaphoreHandle json_sem; 

void create_json(char file_loc[]){
   xSemaphoreTakeRecursive(json_sem, portMAX_DELAY);
   float arr[] = {0};
   json_fprintf(file_loc, "{in_temp_now: %0.1f, in_press_now: %0.1f, main: %Q,"
               " out_temp: %0.01f, out_press: %0.01f, out_hum: %0.01f, pm25_hour: %M,"
               " pm10_hour: %M, aqi_hour: %M, time: %M, refresh_cnt: %d}", 0.0, 0.0, "",
               0.00, 0.00, 0.00, json_printf_array, arr, sizeof(arr), sizeof(arr[0]),"%d",
               json_printf_array, arr, sizeof(arr), sizeof(arr[0]),"%d",
               json_printf_array, arr, sizeof(arr), sizeof(arr[0]),"%d",
               json_printf_array, arr, sizeof(arr), sizeof(arr[0]),"%d", 0);
   ESP_LOGI(TAG, "Writting json file to spiffs");
   xSemaphoreGiveRecursive(json_sem);
}

void create_json_settings(char file_loc[]){

   json_fprintf(file_loc, "{api_key=%d, location=%d, units=%Q}", 0,0,"metrics");
   ESP_LOGI(TAG, "Writting settings json file to spiffs");  
}

void log_data_array(char file_loc[], char key[], char value[], int max_length){
   xSemaphoreTakeRecursive(json_sem, portMAX_DELAY);
   char buf[25];
   strcpy(buf, key);
   char *json_str = json_fread(file_loc);
   FILE *fp = fopen(file_loc, "wb");
   
   int refresh_cnt = 0;
   json_scanf(json_str, strlen(json_str), "{refresh_cnt: %d}", &refresh_cnt);
   if(refresh_cnt <= max_length){
      if(fp != NULL){
         struct json_out out = JSON_OUT_FILE(fp);
         if(refresh_cnt == 0){
            strcat(buf, "[0]");
            json_setf(json_str, strlen(json_str), &out, buf, value);
         }else{
            strcat(buf, "[]");
            json_setf(json_str, strlen(json_str), &out, buf, value);
         }
      }else{
         ESP_LOGE(TAG,"File does not exist.");
         create_json(file_loc);
      }
   }else{
      create_json(file_loc);
   }
   free(refresh_cnt);
   free(json_str);
   fclose(fp);
   xSemaphoreGiveRecursive(json_sem);

}

void set_data_array(char file_loc[], char key[], char value[], int max_length, int position){
   xSemaphoreTakeRecursive(json_sem, portMAX_DELAY);
   char buf[25]; char position_buf[3];
   strcpy(buf, key);
   char *json_str = json_fread(file_loc);
   FILE *fp = fopen(file_loc, "wb");
   
   int refresh_cnt = 0;
   json_scanf(json_str, strlen(json_str), "{refresh_cnt: %d}", &refresh_cnt);
   if(refresh_cnt <= max_length){
      if(fp != NULL){
         struct json_out out = JSON_OUT_FILE(fp);
         sprintf(position_buf, "%d", position);
         strcat(buf, "[");
         strcat(buf, position_buf);
         strcat(buf, "]");
         ESP_LOGI(TAG, "Position: %s", buf);
         json_setf(json_str, strlen(json_str), &out, buf, value);
      }else{
         ESP_LOGE(TAG,"File does not exist.");
         create_json(file_loc);
      }
   }else{
      create_json(file_loc);
   }
   free(refresh_cnt);
   free(json_str);
   fclose(fp);
   xSemaphoreGiveRecursive(json_sem);

}

void log_data(char file_loc[], char key[], char value[], int max_length){
   xSemaphoreTakeRecursive(json_sem, portMAX_DELAY);
   char *json_str = json_fread(file_loc);
   FILE *fp = fopen(file_loc, "wb");

   int refresh_cnt = 0;
   json_scanf(json_str, strlen(json_str), "{refresh_cnt: %d}", &refresh_cnt);
   ESP_LOGI(TAG, "JSON refresh cnt: %d", refresh_cnt);  

   if(refresh_cnt <= max_length){
      struct json_out out = JSON_OUT_FILE(fp);
      json_setf(json_str, strlen(json_str), &out, key, value);
   }else{
      ESP_LOGE(TAG,"File does not exist.");
      create_json(file_loc);
   }
   free(json_str);
   free(refresh_cnt);
   fclose(fp);
   xSemaphoreGiveRecursive(json_sem);
}

void get_data_array(char file_loc[], char key[], float data_array[]){
   xSemaphoreTakeRecursive(json_sem, portMAX_DELAY);
   FILE *file = fopen(file_loc, "r");
   if(file ==NULL)
   {
      ESP_LOGE(TAG,"File does not exist!");
   }
   else{
      size_t total = 0, used = 0;
      int ret = esp_spiffs_info(NULL, &total, &used);
      if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
      } else {      
      vTaskDelay(1000/portTICK_RATE_MS);
      ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
         char *json_str = json_fread(file_loc);
         struct json_token t;
         float value;
         for (int i = 0; json_scanf_array_elem(json_str, json_str, key, i, &t) > 0; i++) {
            // t.type == JSON_TYPE_OBJECT
            json_scanf(t.ptr, t.len, "%f", &value);  // value is 123, then 345
            data_array[i] = value;
            free(&value);
         }

         free(json_str);
      }
      }   
   fclose(file);
   xSemaphoreGiveRecursive(json_sem);
}

void print_logged_data(char file_loc[]){
   xSemaphoreTakeRecursive(json_sem, portMAX_DELAY);
   FILE *file = fopen(file_loc, "r");
   if(file ==NULL)
   {
      ESP_LOGE(TAG,"File does not exist!");
   }
   else{
      size_t total = 0, used = 0;
      int ret = esp_spiffs_info(NULL, &total, &used);
      if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
      } else {      
      vTaskDelay(1000/portTICK_RATE_MS);
      ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
      }
         char line[256];
         while(fgets(line, sizeof(line), file) != NULL)
         {
            printf(line);
         }
      }
   fclose(file);
   xSemaphoreGiveRecursive(json_sem);
}

void spiffs_init(){
   json_sem = xSemaphoreCreateRecursiveMutex();
   ESP_LOGI(TAG, "==== STARTING SPIFFS ====\n");

   esp_vfs_spiffs_conf_t conf = {
   .base_path = "/spiffs",
   .partition_label = NULL,
   .max_files = 5,
   .format_if_mount_failed = true
   };
   
   // Use settings defined above to initialize and mount SPIFFS filesystem.
   // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
   esp_err_t ret = esp_vfs_spiffs_register(&conf);

   if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
         ESP_LOGE(TAG, "Failed to mount or format filesystem");
      } else if (ret == ESP_ERR_NOT_FOUND) {
         ESP_LOGE(TAG, "Failed to find SPIFFS partition");
      } else {
         ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
      }
   }

   size_t total = 0, used = 0;
   ret = esp_spiffs_info(NULL, &total, &used);
   if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
   } else {
      ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
   }
}

void spiffs_terminate(){
   esp_vfs_spiffs_unregister(NULL);
}