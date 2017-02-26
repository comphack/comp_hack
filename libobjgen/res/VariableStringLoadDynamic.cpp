// Load a string with a size specified.
([&]() -> bool
{
    @LENGTH_TYPE@ len;
    @STREAM@.read(reinterpret_cast<char*>(&len),
        sizeof(len));

    if(!@STREAM@.good())
    {
        return false;
    }

    if(0 == len)
    {
        return true;
    }

    char *szValue = new char[len + 1];
    szValue[len] = 0;

    @STREAM@.read(szValue, len);

    if(!@STREAM@.good())
    {
        delete[] szValue;
        return false;
    }

    @SET_CODE@

    delete[] szValue;

    return @STREAM@.good();
})()