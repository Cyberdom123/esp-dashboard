
#ifndef SIMPLE_FORECAST
#define SIMPLE_FORECAST

#define A1  -42.33
#define A2  -4.32
#define A3  52.41
#define A4  1334.26
#define A5  -332.73
#define A6  -34.51
#define A0  -50.90

#define MAGINTUDE   985.75

float predict_pm10(float out_temp, float out_hum, float press,
                    float pm10_prev1, float pm10_prev2, float pm10_prev3);

#endif