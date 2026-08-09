printl("===== CL_NRandom TEST =====");

// Uniform distributions
printl("Int(1, 100)       = " + CL_NRandom.Int(1, 100));
printl("Float(0, 1)       = " + CL_NRandom.Float(0.0, 1.0));
printl("Double(0, 1)      = " + CL_NRandom.Double(0.0, 1.0));
printl("LongDouble(0, 1)  = " + CL_NRandom.LongDouble(0.0, 1.0));

// Gaussian / normal distributions
printl("GaussianFloat     = " + CL_NRandom.GaussianFloat(0.0, 1.0));
printl("GaussianDouble    = " + CL_NRandom.GaussianDouble(0.0, 1.0));
printl("GaussianLongDouble= " + CL_NRandom.GaussianLongDouble(0.0, 1.0));

// Boolean
printl("Bool              = " + CL_NRandom.Bool());

// Character types
printl("Char              = " + CL_NRandom.Char());
printl("SChar             = " + CL_NRandom.SChar());
printl("UChar             = " + CL_NRandom.UChar());
printl("WChar             = " + CL_NRandom.WChar());

// Standard integer types
printl("Short             = " + CL_NRandom.Short());
printl("UShort            = " + CL_NRandom.UShort());
printl("UInt              = " + CL_NRandom.UInt());
printl("Long              = " + CL_NRandom.Long());
printl("ULong             = " + CL_NRandom.ULong());
printl("LongLong          = " + CL_NRandom.LongLong());
printl("ULongLong         = " + CL_NRandom.ULongLong());

// Fixed-width integer types
printl("Int8              = " + CL_NRandom.Int8());
printl("UInt8             = " + CL_NRandom.UInt8());
printl("Int16             = " + CL_NRandom.Int16());
printl("UInt16            = " + CL_NRandom.UInt16());
printl("Int32             = " + CL_NRandom.Int32());
printl("UInt32            = " + CL_NRandom.UInt32());
printl("Int64             = " + CL_NRandom.Int64());
printl("UInt64            = " + CL_NRandom.UInt64());

printl("===== DONE =====");