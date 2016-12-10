([&]() -> @VAR_CODE_TYPE@
{
	try
	{
		return (@VAR_CODE_TYPE@)std::stod(GetXmlText(*@NODE@));
	}
	catch (...)
	{
		status = false;
		return @VAR_CODE_TYPE@{};
	}
})()