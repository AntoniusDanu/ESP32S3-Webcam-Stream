#include <stdio.h>
#include "esp_log.h"
#include "usb/usb_host.h"
#include "libuvc/libuvc.h"
#include "esp_wifi.h"
#include "esp_http_client.h"

static const char *TAG = "UVC_STREAM";

// Callback untuk menangani frame yang diterima
void frame_callback(uvc_frame_t *frame, void *ptr) {
    ESP_LOGI(TAG, "Frame diterima: %u bytes", frame->data_bytes);

    // Konversi YUYV ke format lain jika diperlukan
    // Misalnya, konversi ke RGB atau JPEG sebelum mengirim ke server

    // Kirim frame ke server cloud
    esp_http_client_config_t config = {
        .url = "http://your-server.com/upload",
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_post_field(client, (const char *)frame->data, frame->data_bytes);
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Frame berhasil dikirim, status = %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "Gagal mengirim frame: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void app_main(void) {
    // Inisialisasi Wi-Fi
    // Pastikan ESP32-S3 terhubung ke jaringan Wi-Fi untuk mengirim data ke cloud

    // Inisialisasi USB Host
    usb_host_config_t host_config = {
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    // Inisialisasi UVC
    uvc_context_t *ctx;
    uvc_device_t *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t ctrl;

    ESP_ERROR_CHECK(uvc_init(&ctx, NULL));
    ESP_ERROR_CHECK(uvc_find_device(ctx, &dev, 0, 0, NULL));
    ESP_ERROR_CHECK(uvc_open(dev, &devh));

    // Negosiasi format dan resolusi
    ESP_ERROR_CHECK(uvc_get_stream_ctrl_format_size(
        devh, &ctrl, UVC_FRAME_FORMAT_YUYV, 320, 240, 15));

    // Memulai streaming
    ESP_ERROR_CHECK(uvc_start_streaming(devh, &ctrl, frame_callback, NULL, 0));

    // Biarkan streaming berjalan
    vTaskDelay(portMAX_DELAY);

    // Berhenti streaming dan bersihkan
    uvc_stop_streaming(devh);
    uvc_close(dev);
    uvc_exit(ctx);
    usb_host_uninstall();
}
