#pragma once


class LfsrCorrector
{
public:
    LfsrCorrector();

public:
    char Next(char inBit);

private:
    unsigned __int64 lfsr_;
};