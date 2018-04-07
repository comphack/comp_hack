([&]() -> libobjgen::UUID
{
    return libobjgen::UUID(GetXmlText(*@NODE@));
})()
