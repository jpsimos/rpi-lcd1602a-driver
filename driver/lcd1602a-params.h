#ifndef __LCD1602A_PARAMS
#define __LCD1602A_PARAMS
    static u8 PIN_RS = 17;
    static u8 PIN_RW = 18;
    static u8 PIN_E = 27;
    static u8 PIN_DB0 = 22;
    static u8 PIN_DB1 = 23;
    static u8 PIN_DB2 = 24;
    static u8 PIN_DB3 = 25;
    static u8 PIN_DB4 = 5;
    static u8 PIN_DB5 = 6;
    static u8 PIN_DB6 = 12;
    static u8 PIN_DB7 = 16;

    module_param(PIN_RS, byte, 0);
    module_param(PIN_RW, byte, 0);
    module_param(PIN_DB0, byte, 0);
    module_param(PIN_DB1, byte, 0);
    module_param(PIN_DB2, byte, 0);
    module_param(PIN_DB3, byte, 0);
    module_param(PIN_DB4, byte, 0);
    module_param(PIN_DB5, byte, 0);
    module_param(PIN_DB6, byte, 0);
    module_param(PIN_DB7, byte, 0);

/*
    MODULE_PARAM_DESC(PIN_RS, "Register Select Pin (BCM)");
    MODULE_PARAM_DESC(PIN_RW, "Read / Write Pin (BCM)");
    MODULE_PARAM_DESC(PIN_DB0, "Data Pin 0");
    MODULE_PARAM_DESC(PIN_DB1, "Data Pin 1");
    MODULE_PARAM_DESC(PIN_DB2, "Data Pin 2");
    MODULE_PARAM_DESC(PIN_DB3, "Data Pin 3");
    MODULE_PARAM_DESC(PIN_DB4, "Data Pin 4");
    MODULE_PARAM_DESC(PIN_DB5, "Data Pin 5");
    MODULE_PARAM_DESC(PIN_DB6, "Data Pin 6");
    MODULE_PARAM_DESC(PIN_DB7, "Data Pin 7");
*/
#endif