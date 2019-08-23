
// Open a command prompt and use this command to update it
// Of course changing the IP for this device and the filename & location if need be
// This example the bin file is in the build folder. 
// curl 192.168.0.3:8032 --data-binary @- < build/OTA_Template.bin

#include "Header.h"


TaskHandle_t xTaskList[20];
uint8_t xtaskListCounter = 0;

xTaskHandle TaskHandle_IdleLoop;
 /* -----------------------------------------------------------------------------
   SuspendAllThreads(void)

   Notes:  Used to Kill all threads, mainly for OTA updates when trigered
  -----------------------------------------------------------------------------*/
 void KillAllThreads(void)
 {
    uint8_t list;

    printf("\nKilling A Total of %u Threads\r\n", xtaskListCounter);

    for(list=0; list < xtaskListCounter; list++)
    {
      // Use the handle to delete the task.
      if( xTaskList[list] != NULL )
      {
          printf("Killed Task[%u] Complete\r\n", list);
          vTaskDelete( xTaskList[list] );
      }
      else
      {
          printf("Could not Kill Task[%u] \r\n", list);
      }
    }
 }
/********************************************************************
 *
 *
 *  	idleloop()
 *
 *
 ********************************************************************/
void idleloop(void * parameter)
{
	uint32_t x = 0;
	
	for (;;)
	{
		printf("OTA Template Running: %u\r\n", x++);
		
		vTaskDelay(250 / portTICK_PERIOD_MS);
	}
}
/***********************************************************************
 *
 * app_main
 *
 ***********************************************************************/
void app_main()
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	

	printf("\r\n***************************\r\n");
	printf("***   ESP32 System Up   **\r\n");
	printf("***************************\r\n");

	printf("Compiled at:");
	printf(__TIME__);
	printf(" ");
	printf(__DATE__);
	printf("\r\n");
	
	
	printf("\r\n\r\nStarting WiFi With OTA\r\n");

	// Start WiFi
	initialise_wifi();

	// Start OTA Server
	xTaskCreate(&ota_server_task, "ota_server_task", 4096, NULL, 15, NULL);
	
	// Wait for the WiFi to connect to router
	xEventGroupWaitBits(ota_event_group, OTA_CONNECTED_BIT, false, true, portMAX_DELAY);
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	
	
	printf("\r\n\r\n");
	
	//Spin up Task
	xTaskCreate(idleloop, "idleloop", 2048, NULL, 5, &TaskHandle_IdleLoop); 
	xTaskList[xtaskListCounter++] = TaskHandle_IdleLoop;
	
}