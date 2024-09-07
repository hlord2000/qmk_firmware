Check out 'quantum/via.c' for where all raw HID commands are called from, specifically the function 'raw_hid_receive'
The 'raw_hid_receive' function eventually calls the 'via_custom_value_command_kb' function defined in 'module.c'
There is a default '0x09' Via constant that is required to be in position zero of any incoming buffer. 
The second byte is the command code, as defined in 'keyboards/toffee_studio/module/rawhid/module_raw_hid.h'
All other structs for custom raw HID commands are defined in 'module_raw_hid.h'

An array of function pointers is defined in '...module/rawhid/module_raw_hid.c' Each function pointer's index corresponds to a command code.
'module_raw_hid_parse_packet' is called in 'via_custom_value_command_kb' which subsequently calls its packet parsing command.

Other changes are made in 'lib' where littlefs and lvgl were updated.

Littlefs also necessitated a change to chibios where a means of writing to non-volatile memory was created + glued to littlefs.
This can be found under 'platforms/chibios/drivers/littlefs/file_system.c'

Some of these functions are called within 'module.c' and setup the lfs file system at boot (if required)
