([&]() -> libcomp::String
{
    auto s = libcomp::String(GetXmlText(*@NODE@)).Replace("&#10;", "\r");

#if @FIXED_LENGTH@
    if(@FIXED_LENGTH@ && @SIZE_CHECK@)
    {
        LOG_ERROR(libcomp::String("String is too long and may not load: %1\n").Arg(s));
        LOG_ERROR(libcomp::String("String is %1 bytes when encoded but has to be under %2 bytes.\n").Arg(s.Size()).Arg(@FIXED_LENGTH@));
    }
#endif

    return s;
})()
