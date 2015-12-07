#ifndef DATABASE_H_
#define DATABASE_H_

#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>


class Database
{
private:
    static int row_number_temp;
    static int row_number_pressure;
    static int row_number_wind_speed;
    double total_temp;
    double total_wind_speed;
    double total_press;
    double min_next;
    double max_next;

public:
    void add_pressure(unit_pressure)
    {
        total_press+=unit_pressure;
    }
    void add_temp(unit_temp)
    {
        total_temp+=unit_temp;
    }
    void add_wind_speed(unit_wind_speed)
    {
        total_wind_speed+=unit_wind_speed;
    }
    void add_row_temp(unit_row)
    {
        row_number_temp+=unit_row;
    }
    void add_row_pressure(unit_row)
    {
        row_number_pressure+=unit_row;
    }
    void add_row_wind_speed(unit_row)
    {
        row_number_wind_speed+=unit_row;
    }


};

#endif // DATABASE_H_
