#include <stdio.h>
#include<stdlib.h>
#include "libusb.h"

libusb_device* open_kb(uint16_t vendor, uint16_t product);
libusb_device_handle* hand_kb(libusb_device* dev_found);
void send_color(libusb_device_handle* dev_hand, unsigned char endAddr, unsigned char color_data[6][64], int* transed, int index);
void set_color(unsigned char color_kb[6][64], char **color_line, int line_cnt);
void send_mode(libusb_device_handle* dev_hand, unsigned char mode_data[]);
int set_mode(int mode, unsigned char mode_data[8], unsigned char bright, unsigned char speed, unsigned char rotation, unsigned char trig);


int main(int argc, char** argv) {
   libusb_device *dev;
   uint16_t vendorId = 0x048d;
   uint16_t productId = 0xce00;
   libusb_device_handle* dev_hand = NULL;
   struct libusb_config_descriptor *cfg_desc = NULL;
   uint8_t interface_number;
   struct libusb_interface iface;
   struct libusb_endpoint_descriptor epoint;
   struct libusb_transfer* trans;
   unsigned char bright = 0x16; /* between 00-32 Bigger is brighter */
   unsigned char speed = 0x05;  /* between 01-07 Smaller is faster  */
   unsigned char rotation = 0x01;
   unsigned char trig = 0x00;
   int* transed = NULL;
   int mode = 7;

   FILE *main_config = NULL;
   FILE *color_config = NULL;

   unsigned char init_kb[] = {0x08, 0x02, 0x33, 0x00, bright, 0x00, 0x00, 0x00};                  /*         Keyboard Init       */

                                                                                                  /* ----------------------------*/
                                                                                                  /* | Keyboard Modes | Number | */
   unsigned char modes[11][8] = {   {0x08, 0x02, 0x03, 0x05, 0x00, 0x08, 0x01, 0x00},             /* | off            | 0      | */
                                    {0x08, 0x02, 0x33, 0x00, bright, 0x00, 0x00, 0x00},           /* | single         | 1      | */
                                    {0x08, 0x02, 0x02, speed, bright, 0x08, 0x00, 0x00},          /* | fade           | 2      | */
                                    {0x08, 0x02, 0x03, speed, bright, 0x08, rotation, 0x00},      /* | wave           | 3      | */
                                    {0x08, 0x02, 0x04, speed, bright, 0x08, trig, 0x00},          /* | dots           | 4      | */
                                    {0x08, 0x02, 0x05, 0x00, bright, 0x08, 0x00, 0x00},           /* | rainbow        | 5      | */
                                    {0x08, 0x02, 0x06, speed, bright, 0x08, trig, 0x00},          /* | explosion      | 6      | */
                                    {0x08, 0x02, 0x09, speed, bright, 0x08, 0x00, 0x00},          /* | snake          | 7      | */
                                    {0x08, 0x02, 0x0a, speed, bright, 0x08, 0x00, 0x00},          /* | raindrops      | 8      | */
                                    {0x08, 0x02, 0x0e, speed, bright, 0x08, trig, 0x00},          /* | lines          | 9      | */
                                    {0x08, 0x02, 0x11, speed, bright, 0x08, trig, 0x00}   };      /* | firework       | 10     | */
                                                                                                  /* ----------------------------*/

   unsigned char mode_single[] = {0x08, 0x02, 0x33, 0x00, bright, 0x00, 0x00, 0x00};
   unsigned char mode_fade[] = {0x08, 0x02, 0x02, speed, bright, 0x08, 0x00, 0x00};


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
         printf("Found interface: %hhx \n", interface_number);
      }
   }

   for(int i = 0; i < iface.altsetting[0].bNumEndpoints; i++){
      if((iface.altsetting[0].endpoint[i].bEndpointAddress & (1<<7)) == LIBUSB_ENDPOINT_OUT){
         epoint = iface.altsetting[0].endpoint[i];
         printf("Found endpoint: %hhx \n", epoint.bEndpointAddress);
         break;
      }
   }

   if ( (libusb_set_auto_detach_kernel_driver(dev_hand, 1) == LIBUSB_SUCCESS) && (libusb_claim_interface(dev_hand, interface_number) == 0) ){
      printf("Interface %hhx has been claimed successfully \n", interface_number);
   }else{
      printf("Something went wrong while claiming... \n");
      return 1;
   }

   libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, init_kb, sizeof(init_kb), 4000);

   if (argc == 1){
      main_config = fopen("main.conf", "rt");
      fscanf(main_config, "Mode = %d\nBrightness = %hhx\nSpeed = %hhx\nRotation = %hhx\nTrigger = %hhx", &mode, &bright, &speed, &rotation, &trig);

      set_mode(mode, modes[mode], bright, speed, rotation, trig);
      send_mode(dev_hand, modes[mode]);

      fclose(main_config);

      /* send_color(dev_hand, epoint.bEndpointAddress, color_kb, transed, 0); */
   }else{
      if (argv[1][0] == 'c'){
         /*send_color(dev_hand, epoint.bEndpointAddress, color_kb, transed, 0); */

         int size;
         char **color_line = (char **)malloc(1 * sizeof(char *));
         *(color_line + 0) = (char *)malloc(50 * sizeof(char));
         int line_cnt = 0;

         int x_begin, y_begin, x_end, y_end;
         uint8_t color_red, color_green, color_blue;

         /* FILE *color_head = NULL; */

         color_config = fopen("color.conf", "rt");

         for (int i = 0; ( (fgets(*(color_line + i), 50, color_config)!=NULL) ); i++){
            color_line = realloc(color_line, (sizeof(char *))*(i+2));
            *(color_line + (i + 1) ) = (char *)malloc(50 * sizeof(char));
            line_cnt++;
         }

         printf("%d", line_cnt);

         /* set_color(color_kb, color_line, line_cnt); */

         for(int i = 0; i <= 5; i++){
            for(int j = 0; j <= 20; j++){ 
               color_kb[i][j + 0] = 0xaa;
               color_kb[i][j + 21] = 0x36;
               color_kb[i][j + 42] = 0xd4;
            }
         }

         for (int i = 0; i < line_cnt; i++){
            printf("Read: %s", *(color_line + i));

            /*
            for(int j = 0; (*(*(color_line + i) +j) !='\n') && (*(*(color_line + i) +j) !='\0') ; ){
               if ( (*(*(color_line + i) +j) == '[' ) )
            }*/

            sscanf(*(color_line + i), "[%d,%d]-[%d,%d]: #%2x%2x%2x", &x_begin, &y_begin, &x_end, &y_end, &color_red, &color_green, &color_blue);
            printf("Understood: [%d,%d]-[%d,%d]: #%2x%2x%2x \n", x_begin, y_begin, x_end, y_end, color_red, color_green, color_blue);
            for (int y = y_begin; y <= y_end; y++){
               for (int x = x_begin; x <= x_end; x++){
                  color_kb[y][x + 0] = color_red;
                  color_kb[y][x + 21] = color_green;
                  color_kb[y][x + 42] = color_blue;
               }
            }
         }

         send_color(dev_hand, epoint.bEndpointAddress, color_kb, transed, 0); 

         /* color_head = color_config; */

         /*
         fseek(color_config, 0L, SEEK_END);
         size = ftell(color_config);
         rewind(color_config);

         fscanf(color_config, "%s\n", color_line);

         printf("%s", color_line);*/

         /* fscanf(color_config, "[%d,%d]-[%d,%d]: #\n", x_begin, y_begin, x_end, y_end); */
      }else if (argv[1][0] == 'o'){
         for(int i = 0; i <= 5; i++){
            for(int j = 0; j <= 20; j++){ 
               color_kb[i][j + 0] = 0xaa;
               color_kb[i][j + 21] = 0x36;
               color_kb[i][j + 42] = 0xd4;
            }
         }

         /* e1a0db */ 
         
         
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

         send_color(dev_hand, epoint.bEndpointAddress, color_kb, transed, 0);
      }else if (argv[1][0] == 'm') {
         int mode = atoi(argv[2]);

         for(int i = 3; i < argc; i=i+2){
            switch (argv[i][0])
            {
            case 'b':
               bright = strtol(argv[i+1], NULL, 16);
               break;
            case 's':
               speed = strtol(argv[i+1], NULL, 16);
               break;
            case 'r':
               rotation = strtol(argv[i+1], NULL, 16);
               break;
            case 't':
               trig = strtol(argv[i+1], NULL, 16);
               break;
            default:
               break;
            }
         }

         set_mode(mode, modes[mode], bright, speed, rotation, trig);
         send_mode(dev_hand, modes[mode]);
      }
   }

   libusb_close(dev_hand);

   libusb_exit(NULL);
   printf("Exiting... \n");
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
         printf("  Found! \n");
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
      printf("Something went wrong while opening... \n");
      return NULL;
   }else{
      printf("Device has been opened \n");
      return handle;
   }
}

void set_color(unsigned char color_kb[6][64], char **color_line, int line_cnt){
   /* [%d,%d]-[%d,%d]: #%2x%2x%2x */
   char *parse = NULL;

   for (int i; i < line_cnt; i++){
      parse = strtok( *(color_line + i), "[" );
      parse = strtok(NULL, "-");
   }
}

void send_color(libusb_device_handle* dev_hand, unsigned char endAddr, unsigned char color_kb[6][64], int* transed, int index){
   for (unsigned char i = index; i <= 5; i++){
      unsigned char ctrl_buf[] = {0x16, 0x00, i, 0x00, 0x00, 0x00, 0x00, 0x00};

      libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, ctrl_buf, sizeof(ctrl_buf), 4000);

      libusb_interrupt_transfer(dev_hand, endAddr, color_kb[i], 64*sizeof(unsigned char), transed, 4000);  /* Add check later */
   }
}

int set_mode(int mode, unsigned char mode_data[8], unsigned char bright, unsigned char speed, unsigned char rotation, unsigned char trig){
   switch (mode)
   {
   case 3:
   case 4:
   case 6:
   case 9:
   case 10:
      if (mode == 3){
         mode_data[6] = rotation;
      }else{
         mode_data[6] = trig;
      }
   case 2:
   case 7:
   case 8:
      mode_data[3] = speed;
   case 1:
   case 5:
      mode_data[4] = bright;
   case 0:
      return 0;
      break;
   default:
      return 1;
      break;
   }
}

void send_mode(libusb_device_handle* dev_hand, unsigned char mode_data[]){
   libusb_control_transfer(dev_hand, 0x21, 9, 0x300, 1, mode_data, 8*sizeof(unsigned char), 4000);
}