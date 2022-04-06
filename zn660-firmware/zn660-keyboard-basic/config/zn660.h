// left hand pins

#define L_C01 2
#define L_C02 3
#define L_C03 4
#define L_C04 5
#define L_C05 6
#define L_C06 7
#define L_C07 8
#define L_C08 9 // fake

#define L_R01 28
#define L_R02 29
#define L_R03 30
#define L_R04 0
#define L_R05 1

// right hand pins

#define R_C01 2
#define R_C02 3
#define R_C03 4
#define R_C04 5
#define R_C05 6
#define R_C06 7
#define R_C07 8
#define R_C08 9

#define R_R01 28
#define R_R02 29
#define R_R03 30
#define R_R04 0
#define R_R05 1

#ifdef COMPILE_LEFT

#define PIPE_NUMBER 0

#define C01 L_C01
#define C02 L_C02
#define C03 L_C03
#define C04 L_C04
#define C05 L_C05
#define C06 L_C06
#define C07 L_C07
#define C08 L_C08
#define C09 L_C09

#define R01 L_R01
#define R02 L_R02
#define R03 L_R03
#define R04 L_R04
#define R05 L_R05

#endif

#ifdef COMPILE_RIGHT

#define PIPE_NUMBER 1

#define C01 R_C01
#define C02 R_C02
#define C03 R_C03
#define C04 R_C04
#define C05 R_C05
#define C06 R_C06
#define C07 R_C07
#define C08 R_C08
#define C09 R_C09

#define R01 R_R01
#define R02 R_R02
#define R03 R_R03
#define R04 R_R04
#define R05 R_R05

#endif

#define COLUMNS 8
#define DOBCOLUMNS 16
#define ROWS 5

// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC      {.source        = NRF_CLOCK_LF_SRC_XTAL,            \
                                 .rc_ctiv       = 0,                                \
                                 .rc_temp_ctiv  = 0,                                \
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}
