#include <stdio.h>
#include "libusb.h"

libusb_device* open_kb(uint16_t vendor, uint16_t product);
libusb_device_handle* hand_kb(libusb_device* dev_found);
void wait_end(int* test);
void send_color(libusb_device_handle* dev_hand, unsigned char endAddr, unsigned char data[6][64], int* transed, int index);

int main() {
   libusb_device *dev;
   uint16_t vendorId = 0x048d;
   uint16_t productId = 0xce00;
   libusb_device_handle* dev_hand = NULL;
   struct libusb_config_descriptor *cfg_desc = NULL;
   uint8_t interface_number;
   struct libusb_interface iface;
   struct libusb_endpoint_descriptor epoint;
   struct libusb_transfer* trans;
   unsigned char bright = 0x16;
   int trans_test = 0;
   int* transed = NULL;

   unsigned char init_kb[] = {0x08, 0x02, 0x33, 0x00, bright, 0x00, 0x00, 0x00};
   unsigned char fade[] = {0x08, 0x02, 0x02, 0x05, bright, 0x08, 0x00, 0x00};
   unsigned char color_kb[6][64];

   libusb_init(NULL);
   

   dev = open_kb(vendorId, productId);
   if (dev == NULL){
      return 1;
   }

   dev_hand = hand_kb(dev);
   if (dev_hand == NULL){
      return 1;
   }

   libusb_get_active_config_descriptor(dev, &cfg_desc);

   for(int i = 0; i < cfg_desc->bNumInterfaces; i++){
      if (cfg_desc->interface[i].altsetting[0].bNumEndpoints > 0) {
         interface_number = cfg_desc->interface[i].altsetting[0].bInterfaceNumber;
         iface = cfg_desc->interface[interface_number];
      }
   }

   for(int i = 0; i < iface.altsetting[0].bNumEndpoints; i++){
      if(iface.altsetting[0].endpoint[i].bEndpointAddress & (1<<7) == LIBUSB_ENDPOINT_OUT){
         epoint = iface.altsetting[0].endpoint[i];
         break;
      }
   }

   if ( (libusb_set_auto_detach_kernel_driver(dev_hand, 1) == LIBUSB_SUCCESS) && (libusb_claim_interface(dev_hand, interface_number) == 0) ){
      printf("Interface has been claimed successfully");
   }else{
      printf("Something went wrong while claiming...");
      return 1;
   }

   libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, init_kb, sizeof(init_kb), 4000);

   /* libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, fade, sizeof(fade), 4000); */

   for(int i = 0; i <= 5; i++){
      for(int j = 0; j <= 20; j++){
         /* e1a0db */
         color_kb[i][j + 0] = 0xaa;
         color_kb[i][j + 21] = 0x36;
         color_kb[i][j + 42] = 0xd4;
      }
   }
   color_kb[3][4] = 0xdc;
   color_kb[2][3] = 0xdc;
   color_kb[2][4] = 0xdc;
   color_kb[2][5] = 0xdc;
   color_kb[1][15] = 0xdc;
   color_kb[0][14] = 0xdc;
   color_kb[0][15] = 0xdc;
   color_kb[0][16] = 0xdc;

   color_kb[3][4 + 21] = 0x7f;
   color_kb[2][3 + 21] = 0x7f;
   color_kb[2][4 + 21] = 0x7f;
   color_kb[2][5 + 21] = 0x7f;
   color_kb[1][15 + 21] = 0x7f;
   color_kb[0][14 + 21] = 0x7f;
   color_kb[0][15 + 21] = 0x7f;
   color_kb[0][16 + 21] = 0x7f;

   color_kb[3][4 + 42] = 0x32;
   color_kb[2][3 + 42] = 0x32;
   color_kb[2][4 + 42] = 0x32;
   color_kb[2][5 + 42] = 0x32;
   color_kb[1][15 + 42] = 0x32;
   color_kb[0][14 + 42] = 0x32;
   color_kb[0][15 + 42] = 0x32;
   color_kb[0][16 + 42] = 0x32;

   /*
   unsigned char ctrl_buf[] = {0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

   libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, ctrl_buf, sizeof(ctrl_buf), 4000);

   libusb_interrupt_transfer(dev_hand, epoint.bEndpointAddress, color_kb[0], 64*sizeof(unsigned char), transed, 4000);

   */

   /* send_color(dev_hand, epoint.bEndpointAddress, color_kb, transed, 0); */

   
   for (unsigned char i = 0; i <= 5; i++){
      unsigned char ctrl_buf[] = {0x16, 0x00, i, 0x00, 0x00, 0x00, 0x00, 0x00};

      libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, ctrl_buf, sizeof(ctrl_buf), 4000);

      printf("\n %d", libusb_interrupt_transfer(dev_hand, 0x02, color_kb[i], 64*sizeof(unsigned char), transed, 4000));
   }
   

   /*
   trans = libusb_alloc_transfer(0);

   libusb_fill_control_setup();

   libusb_fill_control_transfer(trans, dev_hand, init_kb, wait_end, &trans_test, 4000); 
   */

   libusb_close(dev_hand);

   libusb_exit(NULL);
   printf("Hello, World!");
   return 0;
}

libusb_device* open_kb(uint16_t vendor, uint16_t product){
   libusb_device **list;
   libusb_device *found = NULL;
   ssize_t cnt = libusb_get_device_list(NULL, &list);
   ssize_t i = 0;

   printf("Attempting to find device...\n");

   for (i = 0; i < cnt; i++) {
      struct libusb_device_descriptor desc;
      struct libusb_device *device = list[i];
      libusb_get_device_descriptor(device, &desc);

      printf("Try %04X:%04X", desc.idVendor, desc.idProduct);
      if ( (desc.idVendor == vendor) && (desc.idProduct == product) ){
         printf("  Found!");
         found = device;
         break;
      }else{
         printf("  Nope");
      }
      printf("\n");
   }
   
   libusb_free_device_list(list, 1);

   if (found){
      return found;
   }else{
      return NULL;
   }
}

libusb_device_handle* hand_kb(libusb_device* dev_found){
   libusb_device_handle *handle;
   libusb_device *found = dev_found;
   int err;

   err = libusb_open(found, &handle);
   if (err){
      printf("Something went wrong while opening...");
      return NULL;
   }else{
      printf("Device has been opened");
      return handle;
   }
}

void wait_end(int* test){
   *test = 1;
}

void send_color(libusb_device_handle* dev_hand, unsigned char endAddr, unsigned char data[6][64], int* transed, int index){
   unsigned char ctrl_buf[] = {0x16, 0x00, index, 0x00, 0x00, 0x00, 0x00, 0x00};

   libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, ctrl_buf, sizeof(ctrl_buf), 4000);

   libusb_bulk_transfer(dev_hand, endAddr, data[index], sizeof(data[index]), transed, 4000);

   if (index + 1 < 6){
      send_color(dev_hand, endAddr, data, transed, index + 1);
   }
}