#include "simple_forecast.h"
//averageAirTemp	averageRelativeHumidity	averageAirPressure	s1_averagePm10	s2_averagePm10	s3_averagePm10
float predict_pm10(float out_temp,float out_hum,float press,float pm10_prev1,float pm10_prev2,float pm10_prev3){
    out_temp = out_temp/MAGINTUDE;
    out_hum = out_hum/MAGINTUDE;
    press = press/MAGINTUDE;
    pm10_prev1 = pm10_prev1/MAGINTUDE;
    pm10_prev2 = pm10_prev2/MAGINTUDE;
    pm10_prev3 = pm10_prev3/MAGINTUDE;

         
    return A1 * out_temp + A2 * out_hum + A3 * press + A4 * pm10_prev1 + A5 * pm10_prev2 + A6 * pm10_prev3 + A0;
}