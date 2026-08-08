printl("========================================");
printl("          NMath VScript Test");
printl("========================================");

// Basic arithmetic
printl("Add(10, 5) = " + ScriptNMathAdd(10.0, 5.0));
printl("Subtract(10, 5) = " + ScriptNMathSubtract(10.0, 5.0));
printl("Multiply(10, 5) = " + ScriptNMathMultiply(10.0, 5.0));
printl("Divide(10, 5) = " + ScriptNMathDivide(10.0, 5.0));

// Powers and roots
printl("Square(5) = " + ScriptNMathSquare(5.0));
printl("Cube(5) = " + ScriptNMathCube(5.0));
printl("Power(2, 8) = " + ScriptNMathPower(2.0, 8.0));
printl("Sqrt(81) = " + Sqrt(81.0));

// Utility
printl("Abs(-25) = " + Abs(-25.0));
printl("Min(10, 20) = " + Min(10.0, 20.0));
printl("Max(10, 20) = " + Max(10.0, 20.0));
printl("Clamp(150, 0, 100) = " + Clamp(150.0, 0.0, 100.0));

// Rounding
printl("Floor(5.9) = " + Floor(5.9));
printl("Ceil(5.1) = " + Ceil(5.1));
printl("Round(5.6) = " + ScriptNMathRound(5.6));

// Trigonometry
local radians = Deg2Rad(90.0);

printl("Deg2Rad(90) = " + radians);
printl("Sin(90 degrees) = " + Sin(radians));
printl("Cos(90 degrees) = " + Cos(radians));
printl("Tan(45 degrees) = " + Tan(Deg2Rad(45.0)));
printl("Rad2Deg(PI) = " + Rad2Deg(ScriptNMathGetPi()));

// Logarithms and exponentials
printl("Log(e) = " + ScriptNMathLog(ScriptNMathGetEulersNumber()));
printl("Log10(1000) = " + ScriptNMathLog10(1000.0));
printl("Exp(1) = " + ScriptNMathExp(1.0));

// Constants
printl("Pi = " + ScriptNMathGetPi());
printl("Euler's number = " + ScriptNMathGetEulersNumber());

// Random functions
printl("RandomInt(1, 100) = " + RandomInt(1, 100));
printl("RandomFloat(0, 1) = " + RandomFloat(0.0, 1.0));
printl("RandomDouble(0, 100) = " + RandomDouble(0.0, 100.0));
printl("RandomLongDouble(0, 100) = " + RandomLongDouble(0.0, 100.0));

printl("RandomGaussianFloat(0, 1) = " +
	RandomGaussianFloat(0.0, 1.0));

printl("RandomGaussianDouble(0, 1) = " +
	RandomGaussianDouble(0.0, 1.0));

printl("RandomGaussianLongDouble(0, 1) = " +
	RandomGaussianLongDouble(0.0, 1.0));

printl("RandomBool() = " + RandomBool());

// Character tests
local randomLetterCode = RandomChar(65, 90);

printl("RandomChar(65, 90) code = " + randomLetterCode);
printl("RandomChar(65, 90) character = " + randomLetterCode.tochar());

printl("RandomSChar(-100, 100) = " + RandomSChar(-100, 100));
printl("RandomUChar(0, 255) = " + RandomUChar(0, 255));
printl("RandomWChar(65, 90) = " + RandomWChar(65, 90));

// Standard integer types
printl("RandomShort(-1000, 1000) = " +
	RandomShort(-1000, 1000));

printl("RandomUShort(0, 60000) = " +
	RandomUShort(0, 60000));

printl("RandomUInt(0, 2000000000) = " +
	RandomUInt(0, 2000000000));

printl("RandomLong(-100000, 100000) = " +
	RandomLong(-100000, 100000));

printl("RandomULong(0, 2000000000) = " +
	RandomULong(0, 2000000000));

printl("RandomLongLong(-1000000, 1000000) = " +
	RandomLongLong(-1000000, 1000000));

printl("RandomULongLong(0, 2000000000) = " +
	RandomULongLong(0, 2000000000));

// Fixed-width integers
printl("RandomInt8(-128, 127) = " +
	RandomInt8(-128, 127));

printl("RandomUInt8(0, 255) = " +
	RandomUInt8(0, 255));

printl("RandomInt16(-32768, 32767) = " +
	RandomInt16(-32768, 32767));

printl("RandomUInt16(0, 65535) = " +
	RandomUInt16(0, 65535));

printl("RandomInt32(-100000, 100000) = " +
	RandomInt32(-100000, 100000));

printl("RandomUInt32(0, 2000000000) = " +
	RandomUInt32(0, 2000000000));

printl("RandomInt64(-1000000, 1000000) = " +
	RandomInt64(-1000000, 1000000));

printl("RandomUInt64(0, 2000000000) = " +
	RandomUInt64(0, 2000000000));

printl("========================================");
printl("        NMath VScript Test Done");
printl("========================================");