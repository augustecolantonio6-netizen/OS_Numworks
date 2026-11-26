# Upstream registers these components as applications. AOS compiles the USB
# view and battery widget directly, so they need only palette and message metadata.
$(call object_for,aos/main.cpp aos/usb_screen.cpp apps/usb/usb_connected_controller.cpp apps/battery_view.cpp): $(BUILD_DIR)/escher/palette.h $(BUILD_DIR)/apps/i18n.h
