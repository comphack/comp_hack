// Save a string with a size specified.
([&]() -> bool
{
    std::stringstream @ENCODESTREAM@;
    
    @ENCODE_CODE@
    
    if(@STREAM@.good())
    {
        @LENGTH_TYPE@ len = static_cast<@LENGTH_TYPE@>(value.size());
        @STREAM@.write(reinterpret_cast<const char*>(&len),
            sizeof(len));
        
        if(@STREAM@.good() && len > 0)
        {
            @STREAM@ << @ENCODESTREAM@.rdbuf();
        }
    }

    return @STREAM@.good();
})()