idf_component_register(
    SRCS "main.c"
         "appfs_wrapper.c"
         "settings.c"
         "system_wrapper.c"
         "wifi_ota.c"
         "bootscreen.c"
         "menus/hatchery.c"
         "menus/settings.c"
         "menus/start.c"
         "menus/dev.c"
         "menus/wifi.c"
         "menus/sao.c"
         "menus/launcher.c"
         "nametag.c"
         "file_browser.c"
         "test_common.c"
         "factory_test.c"
         "button_test.c"
         "wifi_test.c"
         "sao_eeprom.c"
         "rtc_memory.c"
         "metadata.c"
         "wifi_defaults.c"
         "wifi_cert.c"
         "http_download.c"
         "filesystems.c"
         "app_management.c"
         "app_update.c"
    INCLUDE_DIRS "."
                 "include"
                 "menus"
    EMBED_TXTFILES ${project_dir}/resources/isrgrootx1.pem
                   ${project_dir}/resources/custom_ota_cert.pem
    EMBED_FILES ${project_dir}/resources/fri3d2022_logo.png
                ${project_dir}/resources/icons/dev.png
                ${project_dir}/resources/icons/home.png
                ${project_dir}/resources/icons/settings.png
                ${project_dir}/resources/icons/apps.png
                ${project_dir}/resources/icons/hatchery.png
                ${project_dir}/resources/icons/tag.png
                ${project_dir}/resources/icons/bitstream.png
                ${project_dir}/resources/icons/python.png
                ${project_dir}/resources/icons/hourglass.png
                ${project_dir}/resources/icons/update.png
)
