#include "usb_names.h"

// USED BY TEENSY TO DEFINE USB PRODUCT/MANUFACTURER/SERIAL INFO

#define PRODUCT_NAME      {'n','e', 'o', 'n','o','m','e'}
#define PRODUCT_NAME_LEN  4
#define MANUFACTURER_NAME  {'m','o','n','o','m','e'}
#define MANUFACTURER_NAME_LEN 6

// Change this to something different if you like - but keep the "m" as the first character.
#define SERIAL_NUM  {'m','9','0','6','7','8','4','5'}
#define SERIAL_NUM_LEN 8

struct usb_string_descriptor_struct usb_string_product_name = {
  2 + PRODUCT_NAME_LEN * 2,
  3,
  PRODUCT_NAME
};

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  2 + MANUFACTURER_NAME_LEN * 2,
  3,
  MANUFACTURER_NAME
};

struct usb_string_descriptor_struct usb_string_serial_number = {
  2 + SERIAL_NUM_LEN * 2,
  3,
  SERIAL_NUM
};
