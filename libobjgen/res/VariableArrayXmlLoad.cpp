([&]() -> @VAR_CODE_TYPE@
{
	@VAR_CODE_TYPE@ arr;
	
	auto elements = GetXmlChildren(*@NODE@, "elements");
	if(elements.size() < @ELEMENT_COUNT@)
	{
		for(int i = 0; i < elements.size(); i++)
		{
			auto element = elements[i];
			arr[i] = @ELEMENT_ACCESS_CODE@;
		}
	}
	else
	{
		status = false;
	}
	
	return arr;
})()