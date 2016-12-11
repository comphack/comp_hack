([&]() -> @VAR_CODE_TYPE@
{
	@VAR_CODE_TYPE@ arr;
	
	auto elements = GetXmlChildren(*@NODE@, "element");
	if(elements.size() < @ELEMENT_COUNT@)
	{
		for(int i = 0; i < @ELEMENT_COUNT@; i++)
		{
			if(i >= elements.size())
			{
				arr[i] = @DEFAULT_VALUE@;
			}
			else
			{
				auto element = elements[i];
				arr[i] = @ELEMENT_ACCESS_CODE@;
			}
		}
	}
	else
	{
		status = false;
	}
	
	return arr;
})()