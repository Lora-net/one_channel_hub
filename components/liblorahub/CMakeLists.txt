set(liblorahub "lorahub_aux.c" "lorahub_hal.c" "lorahub_hal_rx.c" "lorahub_hal_tx.c" "lr11xx_driver_extension.c")

idf_component_register(SRCS "${liblorahub}"
                       REQUIRES esp_timer
                       PRIV_REQUIRES driver smtc_ral
                       INCLUDE_DIRS "." "../radio_drivers" "../smtc_ral/src")
