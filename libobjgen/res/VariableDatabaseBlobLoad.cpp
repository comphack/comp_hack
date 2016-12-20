([&]()
{
    std::vector<char> value;

    if(!query.GetValue(@COLUMN_NAME@, value))
    {
        return false;
    }

    libcomp::VectorStream<char> vstream(value);
    std::istream stream(&vstream);

    return @LOAD_CODE@;
}())
