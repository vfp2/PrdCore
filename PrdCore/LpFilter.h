#pragma once


class LpFilter
{
public:
    void Init(double initVal);
    double Feed(double newVal, double length);
    double GetValue();

private:
    double filteredVal_;
};
