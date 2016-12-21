// Save a string with a size specified.
([&]() -> bool
{
    @LENGTH_TYPE@ len = static_cast<@LENGTH_TYPE@>(@VAR_NAME@.Length());
    @STREAM@.stream.write(reinterpret_cast<const char*>(&len),
        sizeof(len));

	if(@STREAM@.stream.good())
	{
		@ENCODE_CODE@
	}

    return @STREAM@.stream.good();
})()