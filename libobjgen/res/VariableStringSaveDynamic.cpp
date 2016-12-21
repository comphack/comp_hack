// Save a string with a size specified.
([&]() -> bool
{
    std::stringstream @ENCODESTREAM@;
    
    @ENCODE_CODE@
    
    if(@STREAM@.stream.good())
    {
        @LENGTH_TYPE@ len = static_cast<@LENGTH_TYPE@>(value.size());
        @STREAM@.stream.write(reinterpret_cast<const char*>(&len),
            sizeof(len));
        
        if(@STREAM@.stream.good())
        {
            @STREAM@.stream << @ENCODESTREAM@.rdbuf();
        }
    }

    return @STREAM@.stream.good();
})()