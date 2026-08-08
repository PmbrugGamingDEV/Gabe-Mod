#pragma once

//========= Copyright PmbrugGaming. All rights reserved. ============//
//
// Purpose: Defines the public interfaces for the NMath library.
//
// $NoKeywords: $
//
//=============================================================================//

#include <cstdint>
#include <random>


class IMath // Version 1
{
public:

	virtual ~IMath() = default;

    /* Non-virtual Math */

    //--------------------------------------------------------------------------
    // Basic arithmetic
    //--------------------------------------------------------------------------

    // Returns the sum of two values.
    template<typename T> inline T Add(T a, T b) { return a + b; }

    // Returns the difference between two values.
    template<typename T> inline T Subtract(T a, T b) { return a - b; }

    // Returns the product of two values.
    template<typename T> inline T Multiply(T a, T b) { return a * b; }

    // Returns the quotient of two values.
    // Caller is responsible for ensuring b is not zero.
    template<typename T> inline T Divide(T a, T b) { return a / b; }

    //--------------------------------------------------------------------------
    // Powers & roots
    //--------------------------------------------------------------------------

    // Returns x squared (x²).
    template<typename T> inline T Square(T x) { return x * x; }

    // Returns x cubed (x³).
    template<typename T> inline T Cube(T x) { return x * x * x; }

    // Raises a value to the specified power.
    // Example: Power(2, 8) = 256
    template<typename T> inline T Power(T base, T exponent) { return static_cast<T>(std::pow(base, exponent)); }

    // Returns the square root of a value.
    // Input must be non-negative for real-number results.
    template<typename T> inline T SquareRoot(T x) { return static_cast<T>(std::sqrt(x)); }

    //--------------------------------------------------------------------------
    // Utility functions
    //--------------------------------------------------------------------------

    // Returns the absolute (positive) value.
    template<typename T> inline T Absolute(T x) { return static_cast<T>(std::abs(x)); }

    // Returns the smaller of two values.
    template<typename T> inline T Minimum(T a, T b) { return (a < b) ? a : b; }

    // Returns the larger of two values.
    template<typename T> inline T Maximum(T a, T b) { return (a > b) ? a : b; }

    // Restricts a value to the specified range.
    template<typename T> inline T Clamp(T value, T min, T max) { return (value < min) ? min : (value > max ? max : value); }

	//--------------------------------------------------------------------------
	// Rounding
	//--------------------------------------------------------------------------

	// Rounds a value down toward negative infinity.
	virtual double Floor(double x) const = 0;

	// Rounds a value up toward positive infinity.
	virtual double Ceiling(double x) const = 0;

	// Rounds a value to the nearest integer.
	virtual double Round(double x) const = 0;

	//--------------------------------------------------------------------------
	// Trigonometry (angles in radians)
	//--------------------------------------------------------------------------

	// Returns the sine of an angle in radians.
	virtual double Sin(double radians) const = 0;

	// Returns the cosine of an angle in radians.
	virtual double Cos(double radians) const = 0;

	// Returns the tangent of an angle in radians.
	virtual double Tan(double radians) const = 0;

	//--------------------------------------------------------------------------
	// Angle conversion
	//--------------------------------------------------------------------------

	// Converts degrees to radians.
	virtual double DegreesToRadians(double degrees) const = 0;

	// Converts radians to degrees.
	virtual double RadiansToDegrees(double radians) const = 0;

	//--------------------------------------------------------------------------
	// Logarithms & exponentials
	//--------------------------------------------------------------------------

	// Returns the natural logarithm (base e).
	virtual double Log(double x) const = 0;

	// Returns the common logarithm (base 10).
	virtual double Log10(double x) const = 0;

	// Returns Euler's number (e) raised to the specified power.
	virtual double Exp(double x) const = 0;

	//--------------------------------------------------------------------------
	// Mathematical constants
	//--------------------------------------------------------------------------

	// Returns the value of π (pi).
	virtual double GetPi() const = 0;

	// Returns the value of Euler's number (e).
	virtual double GetEulersNumber() const = 0;
};

class IRandom // Version 2
{
public:

  virtual ~IRandom() = default;

    //--------------------------------------------------------------------------
     // Uniform distributions
     //--------------------------------------------------------------------------

     // Returns a uniformly distributed random integer in the specified range.
    virtual int RandomInt(int lower, int upper) = 0;

    // Returns a uniformly distributed random float in the specified range.
    virtual float RandomFloat(float lower, float upper) = 0;

    // Returns a uniformly distributed random double in the specified range.
    virtual double RandomDouble(double lower, double upper) = 0;

    // Returns a uniformly distributed random long double in the specified range.
    virtual long double RandomLongDouble(long double lower, long double upper) = 0;

    //--------------------------------------------------------------------------
    // Normal (Gaussian) distributions
    //--------------------------------------------------------------------------

    // Returns a normally distributed random float.
    virtual float RandomGaussianFloat(float mean, float stddev) = 0;

    // Returns a normally distributed random double.
    virtual double RandomGaussianDouble(double mean, double stddev) = 0;

    // Returns a normally distributed random long double.
    virtual long double RandomGaussianLongDouble(long double mean, long double stddev) = 0;

    //--------------------------------------------------------------------------
    // Boolean
    //--------------------------------------------------------------------------

    // Returns either true or false with equal probability.
    virtual bool RandomBool() = 0;

    //--------------------------------------------------------------------------
    // Character types
    //--------------------------------------------------------------------------

    // Returns a random char in the specified range.
    virtual char RandomChar(char lower, char upper) = 0;

    // Returns a random signed char in the specified range.
    virtual signed char RandomSChar(signed char lower, signed char upper) = 0;

    // Returns a random unsigned char in the specified range.
    virtual unsigned char RandomUChar(unsigned char lower, unsigned char upper) = 0;

    // Returns a random wide character in the specified range.
    virtual wchar_t RandomWChar(wchar_t lower, wchar_t upper) = 0;

    //--------------------------------------------------------------------------
    // Standard integer types
    //--------------------------------------------------------------------------

    // Returns a random short integer in the specified range.
    virtual short RandomShort(short lower, short upper) = 0;

    // Returns a random unsigned short integer in the specified range.
    virtual unsigned short RandomUShort(unsigned short lower, unsigned short upper) = 0;

    // Returns a random unsigned integer in the specified range.
    virtual unsigned int RandomUInt(unsigned int lower, unsigned int upper) = 0;

    // Returns a random long integer in the specified range.
    virtual long RandomLong(long lower, long upper) = 0;

    // Returns a random unsigned long integer in the specified range.
    virtual unsigned long RandomULong(unsigned long lower, unsigned long upper) = 0;

    // Returns a random long long integer in the specified range.
    virtual long long RandomLongLong(long long lower, long long upper) = 0;

    // Returns a random unsigned long long integer in the specified range.
    virtual unsigned long long RandomULongLong(unsigned long long lower, unsigned long long upper) = 0;

    //--------------------------------------------------------------------------
    // Fixed-width integer types
    //--------------------------------------------------------------------------

    // Returns a random 8-bit signed integer.
    virtual int8_t RandomInt8(int8_t lower, int8_t upper) = 0;

    // Returns a random 8-bit unsigned integer.
    virtual uint8_t RandomUInt8(uint8_t lower, uint8_t upper) = 0;

    // Returns a random 16-bit signed integer.
    virtual int16_t RandomInt16(int16_t lower, int16_t upper) = 0;

    // Returns a random 16-bit unsigned integer.
    virtual uint16_t RandomUInt16(uint16_t lower, uint16_t upper) = 0;

    // Returns a random 32-bit signed integer.
    virtual int32_t RandomInt32(int32_t lower, int32_t upper) = 0;

    // Returns a random 32-bit unsigned integer.
    virtual uint32_t RandomUInt32(uint32_t lower, uint32_t upper) = 0;

    // Returns a random 64-bit signed integer.
    virtual int64_t RandomInt64(int64_t lower, int64_t upper) = 0;

    // Returns a random 64-bit unsigned integer.
    virtual uint64_t RandomUInt64(uint64_t lower, uint64_t upper) = 0;

    //--------------------------------------------------------------------------
    // random array selection
    //--------------------------------------------------------------------------

    template<typename T>
    T GetRandomFromVecList(const std::vector<T>& array)
    {
        if (array.empty())
        {
            throw std::invalid_argument(
                "GetRandomFromVecList: vector cannot be empty.");
        }

        std::uniform_int_distribution<std::size_t> distribution(
            0,
            array.size() - 1);

        return array[distribution(m_Generator)];
    }

private:
    std::mt19937_64 m_Generator;
};

extern IRandom* g_pRandom; // Global random accessor
extern IMath* g_pMath; // Global math accessor