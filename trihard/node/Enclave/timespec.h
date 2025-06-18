#include <ctime>

timespec standardize(const timespec& ts)
{
    timespec result = ts;
    while(result.tv_nsec < 0)
    {
        result.tv_sec--;
        result.tv_nsec += 1000000000;
    }
    while(result.tv_nsec >= 1000000000)
    {
        result.tv_sec++;
        result.tv_nsec -= 1000000000;
    }
    return result;
}

double to_double(const timespec& ts)
{
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

bool operator!=(const timespec& lhs, const timespec& rhs)
{
    timespec temp_lhs = standardize(lhs);
    timespec temp_rhs = standardize(rhs);
    if(temp_lhs.tv_sec != temp_rhs.tv_sec || temp_lhs.tv_nsec != temp_rhs.tv_nsec)
        return true;
    return false;
}

bool operator==(const timespec& lhs, const timespec& rhs)
{
    return !(lhs != rhs);
}

bool operator<(const timespec& lhs, const timespec& rhs)
{
    timespec temp_lhs = standardize(lhs);
    timespec temp_rhs = standardize(rhs);
    if(temp_lhs.tv_sec < temp_rhs.tv_sec)
        return true;
    else if(temp_lhs.tv_sec == temp_rhs.tv_sec && temp_lhs.tv_nsec < temp_rhs.tv_nsec)
        return true;
    return false;
}

bool operator>(const timespec& lhs, const timespec& rhs)
{
    return rhs < lhs;
}

bool operator<=(const timespec& lhs, const timespec& rhs)
{
    return !(rhs < lhs);
}

bool operator>=(const timespec& lhs, const timespec& rhs)
{
    return !(lhs < rhs);
}

timespec operator+(const timespec& lhs, const timespec& rhs)
{
    timespec result;
    result.tv_sec = lhs.tv_sec + rhs.tv_sec;
    result.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;
    return standardize(result);
}

timespec operator+(const timespec& lhs, const long long& rhs)
{
    timespec result;
    result.tv_sec = lhs.tv_sec;
    result.tv_nsec = lhs.tv_nsec + rhs;
    return standardize(result);
}

timespec operator+(const timespec& lhs, const double& rhs)
{
    timespec result;
    result.tv_sec = (long long)((double)lhs.tv_sec + rhs);
    double second_remainder = (double)lhs.tv_sec + rhs - (double)((long long)((double)lhs.tv_sec + rhs));
    result.tv_nsec = (long long)(second_remainder*1000000000+(double)lhs.tv_nsec);
    return standardize(result);
}

timespec operator-(const timespec& lhs, const timespec& rhs)
{
    timespec result;
    result.tv_sec = lhs.tv_sec - rhs.tv_sec;
    result.tv_nsec = lhs.tv_nsec - rhs.tv_nsec;
    return standardize(result);
}

timespec operator-(const timespec& lhs, const long long& rhs)
{
    timespec result;
    result.tv_sec = lhs.tv_sec;
    result.tv_nsec = lhs.tv_nsec - rhs;
    return standardize(result);
}

timespec operator-(const timespec& lhs)
{
    timespec result;
    result.tv_sec = -lhs.tv_sec;
    result.tv_nsec = -lhs.tv_nsec;
    return standardize(result);
}

timespec operator-(const timespec& lhs, const double& rhs)
{
    timespec result;
    result.tv_sec = (long long)((double)lhs.tv_sec - rhs);
    double second_remainder = (double)lhs.tv_sec - rhs - (double)((long long)((double)lhs.tv_sec - rhs));
    result.tv_nsec = (long long)(second_remainder*1000000000+(double)lhs.tv_nsec);
    return standardize(result);
}

timespec operator*(const timespec& lhs, const double& rhs)
{
    timespec result;
    result.tv_sec = (long long)((double)lhs.tv_sec * rhs);
    double second_remainder = (double)lhs.tv_sec * rhs - (double)((long long)((double)lhs.tv_sec * rhs));
    result.tv_nsec = (long long)(second_remainder*1000000000+(double)lhs.tv_nsec * rhs);
    return standardize(result);
}

timespec operator/(const timespec& lhs, const double& rhs)
{
    timespec result;
    result.tv_sec = (long long)((double)lhs.tv_sec / rhs);
    double sec_remainder = (double)lhs.tv_sec / rhs - (double)((long long)((double)lhs.tv_sec / rhs));
    result.tv_nsec = (long long)(sec_remainder*1000000000+(long long)((double)lhs.tv_nsec / rhs));
    return standardize(result);
}

timespec& operator+=(timespec& lhs, const timespec& rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

timespec& operator+=(timespec& lhs, const long long& rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

timespec& operator+=(timespec& lhs, const double& rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

timespec& operator-=(timespec& lhs, const timespec& rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

timespec& operator-=(timespec& lhs, const long long& rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

timespec& operator-=(timespec& lhs, const double& rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

timespec& operator*=(timespec& lhs, const double& rhs)
{
    lhs = lhs * rhs;
    return lhs;
}
timespec& operator/=(timespec& lhs, const double& rhs)
{
    lhs = lhs / rhs;
    return lhs;
}