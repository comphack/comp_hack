std::string value = @VAR_NAME@.ToUtf8();

if(!value.empty())
{
    @ENCODESTREAM@.write(value.c_str(), static_cast<std::streamsize>(
        value.size()));
}