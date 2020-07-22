#include "ota_utils.h"

static const char *TAG_OTA_SERVER = "ota_utils";

static const esp_partition_t *update_partition;
static esp_ota_handle_t ota_handle;

static int socket_id = 0;

int socket_error_check(const char *step, const int socket)
{
    int result;
    uint32_t optlen = sizeof(int);

    int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);

    if (!err)
    {
        logE(TAG_OTA_SERVER, "Step: %s, Error: %s", step, strerror(result));
    }
    else
    {
        logE(TAG_OTA_SERVER, "getsockopt(...) Failure: %s", strerror(err));
    }

    return result;
}

void send_response(int sck_id, bool flag)
{
    char res_buff[128];
    int send_len = 0;

    if (flag)
    {
        send_len = sprintf(res_buff, "200: OK\n");
        send(socket_id, res_buff, send_len, 0);
    }
    else
    {
        send_len = sprintf(res_buff, "400: Bad Request\n");
        send(socket_id, res_buff, send_len, 0);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    close(socket_id);
}

esp_err_t init_tcp_server()
{
    logD(TAG_OTA_SERVER, "Server Socket: Port - %d", OTA_LISTEN_PORT);

    static bool initial_socket_setup = true;
    static int server_socket = -1;

    if (initial_socket_setup)
    {
        struct sockaddr_in server_addr;
        server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (server_socket < 0)
        {
            socket_error_check("create_server", server_socket);
            return ESP_FAIL;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(OTA_LISTEN_PORT);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            socket_error_check("bind_server", server_socket);
            close(server_socket);
            return ESP_FAIL;
        }

        initial_socket_setup = false;
    }

    if (listen(server_socket, 5) < 0)
    {
        socket_error_check("listen_server", server_socket);
        close(server_socket);
        return ESP_FAIL;
    }

    struct sockaddr_in client_addr;
    unsigned int socklen = sizeof(client_addr);
    socket_id = accept(server_socket, (struct sockaddr *)&client_addr, &socklen);

    if (socket_id < 0)
    {
        socket_error_check("accept_server", socket_id);
        close(server_socket);
        return ESP_FAIL;
    }

    logI(TAG_OTA_SERVER, "%s", "TCP Connection Established");
    return ESP_OK;
}

esp_err_t receive_data(void)
{
    // Receving Buffer
    char ota_buff[OTA_BUFF_SIZE] = {0};
    int recv_len = 0, content_received = 0, content_length = -1;

    bool is_header_packet = true;
    do
    {
        recv_len = recv(socket_id, ota_buff, OTA_BUFF_SIZE, 0);

        if (recv_len > 0)
        {
            if (is_header_packet)
            {
                logD(TAG_OTA_SERVER, "%s", "Received Header Packet");

                const char *content_length_start = "Content-Length: ";
                char *content_length_start_p = strstr(ota_buff, content_length_start) + strlen(content_length_start);
                sscanf(content_length_start_p, "%d", &content_length);
                logD(TAG_OTA_SERVER, "Detected content length: %d", content_length);

                IS_ESP_OK(esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle), EXIT);

                const char *header_end = "\r\n\r\n";
                char *body_start = strstr(ota_buff, header_end) + strlen(header_end);
                int body_len = recv_len - (body_start - ota_buff);

                IS_ESP_OK(esp_ota_write(ota_handle, body_start, body_len), EXIT);
                content_received += body_len;
                is_header_packet = false;
            }
            else
            {
                IS_ESP_OK(esp_ota_write(ota_handle, ota_buff, recv_len), EXIT);
                content_received += recv_len;
            }
        }
        else if (recv_len < 0)
        {
            logE(TAG_OTA_SERVER, "Error in receiving data: %d", errno);
            IS_ESP_OK(ESP_FAIL, EXIT);
        }

    } while (recv_len > 0 && content_received < content_length);
    
    logD(TAG_OTA_SERVER, "OTA Transferred Finished: %d bytes", content_received);
    return ESP_OK;

    EXIT:
        logE(TAG_OTA_SERVER, "%s", "OTA Error");
        return ESP_FAIL;
}

void ota_server_task(void *arg)
{
    while (true)
    {
        IS_ESP_OK(init_tcp_server(), EXIT);
        logD(TAG_OTA_SERVER, "Socket ID: %d", socket_id);

        update_partition = esp_ota_get_next_update_partition(NULL);
        logD(TAG_OTA_SERVER, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, update_partition->address);
        esp_log_level_set("esp_image", ESP_LOG_ERROR);

        IS_ESP_OK(receive_data(), EXIT);

        // Sending Response
        send_response(socket_id, true);

        IS_ESP_OK(esp_ota_end(ota_handle), EXIT);
        IS_ESP_OK(esp_ota_set_boot_partition(update_partition), EXIT);

        const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
        logD(TAG_OTA_SERVER, "Next Boot Partition Subtype %d At Offset 0x%x", boot_partition->subtype, boot_partition->address);

        // Verification of image
        esp_image_metadata_t image_metadata = {0};
        esp_partition_pos_t ota_partition = {.offset = boot_partition->address, .size = boot_partition->size};

        IS_ESP_OK(esp_image_verify(ESP_IMAGE_VERIFY, &ota_partition, &image_metadata), EXIT);

        // Preparing for restart
        esp_wifi_stop();
        esp_wifi_deinit();

        esp_restart();

    EXIT:
        logE(TAG_OTA_SERVER, "%s", "OTA Failed!");
        send_response(socket_id, false);
    }
}