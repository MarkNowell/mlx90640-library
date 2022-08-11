#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "headers/MLX90640_API.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_NONE    "\x1b[30m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// #define FMT_STRING "%+06.2f "
// #define FMT_STRING "\u2588\u2588"
#define FMT_STRING  "   "

#define _Argb(idx, name, r, g, b, h, s, l) { idx, name, r, g, b, h, s, l },
struct { uint8_t idx ; const char *name ; uint8_t r, g, b ; int16_t h, s, l ; } xClrTab[] = {
_Argb(21,"Blue1",0,0,255,240,1000,500)
_Argb(27,"DodgerBlue2",0,95,255,218,1000,500)
_Argb(33,"DodgerBlue1",0,135,255,208,1000,500)
_Argb(39,"DeepSkyBlue1",0,175,255,199,1000,500)
_Argb(45,"Turquoise2",0,215,255,189,1000,500)
_Argb(14,"Aqua*",0,255,255,180,1000,500)
_Argb(51,"Cyan1",0,255,255,180,1000,500)
_Argb(50,"Cyan2",0,255,215,171,1000,500)
_Argb(49,"MediumSpringGreen",0,255,175,161,1000,500)
_Argb(48,"SpringGreen1",0,255,135,152,1000,500)
_Argb(47,"SpringGreen2",0,255,95,142,1000,500)
_Argb(10,"Lime*",0,255,0,120,1000,500)
_Argb(46,"Green1",0,255,0,120,1000,500)
_Argb(82,"Chartreuse2",95,255,0,98,1000,500)
_Argb(118,"Chartreuse1",135,255,0,88,1000,500)
_Argb(154,"GreenYellow",175,255,0,79,1000,500)
_Argb(190,"Yellow2",215,255,0,69,1000,500)
_Argb(11,"Yellow*",255,255,0,60,1000,500)
_Argb(226,"Yellow1",255,255,0,60,1000,500)
_Argb(220,"Gold1",255,215,0,51,1000,500)
_Argb(214,"Orange1",255,175,0,41,1000,500)
_Argb(208,"DarkOrange",255,135,0,32,1000,500)
_Argb(202,"OrangeRed1",255,95,0,22,1000,500)
_Argb(9,"Red*",255,0,0,0,1000,500)
_Argb(196,"Red1",255,0,0,0,1000,500)
} ;
#undef _Argb
const int XClrTabSize = sizeof(xClrTab) / sizeof(*xClrTab) ;

const int MinTemp = 10, MaxTemp = 40 ;

#define MLX_I2C_ADDR 0x33

int main(){
    int state = 0;
    printf("Starting...\n");
    static uint16_t eeMLX90640[832];
    float emissivity = 1;
    uint16_t frame[834];
    static float image[768];
    float eTa;
    static uint16_t data[768*sizeof(float)];

    MLX90640_SetDeviceMode(MLX_I2C_ADDR, 0);
    MLX90640_SetSubPageRepeat(MLX_I2C_ADDR, 0);
    MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b010);
    MLX90640_SetChessMode(MLX_I2C_ADDR);
    //MLX90640_SetSubPage(MLX_I2C_ADDR, 0);
    printf("Configured...\n");

    paramsMLX90640 mlx90640;
    MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640);
    MLX90640_ExtractParameters(eeMLX90640, &mlx90640);

    int refresh = MLX90640_GetRefreshRate(MLX_I2C_ADDR);
    printf("EE Dumped...\n");

    int frames = 30;
    int subpage;
    static float mlx90640To[768];
    while (1){
        state = !state;
        //printf("State: %d \n", state);
        MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
        // MLX90640_InterpolateOutliers(frame, eeMLX90640);
        eTa = MLX90640_GetTa(frame, &mlx90640);
        subpage = MLX90640_GetSubPageNumber(frame);
        MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640To);

        MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, 1, &mlx90640);
        MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, 1, &mlx90640);

        printf("Subpage: %d\n", subpage);
        //MLX90640_SetSubPage(MLX_I2C_ADDR,!subpage);

        /* 
        ESC[38;5;?n?m Select foreground color
        ESC[48;5;?n?m Select background color
        0-  7:  standard colors (as in ESC [ 30–37 m)
        8- 15:  high intensity colors (as in ESC [ 90–97 m)
        16-231:  6 × 6 × 6 cube (216 colors): 16 + 36 × r + 6 × g + b (0 = r, g, b = 5)
        232-255:  grayscale from black to white in 24 steps
        */

        printf("\x1b[38;5;%um", 0) ;        // black foreground
        for(int x = 0; x < 32; x++){
            for(int y = 0; y < 24; y++){
                //std::cout << image[32 * y + x] << ",";
                float temp = mlx90640To[32 * (23-y) + x];
                float val = XClrTabSize * (temp - MinTemp) / (MaxTemp - MinTemp) ;
                int clridx = round(val) ;
                if (clridx < 0) clridx = 0 ;
                else if (clridx >= XClrTabSize) clridx = XClrTabSize - 1 ; 
                int clr = xClrTab[clridx].idx ;     // map to esc code
                printf("\x1b[48;5;%um %02u", clr, (int) temp) ;
            } ;
            printf("\x1b[48;5;%um\n", 0) ;
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(20));
        printf("\x1b[33A");
    }
    return 0;
}



