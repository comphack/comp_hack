@VAR_VALUE_TYPE@ @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@(@VAR_KEY_TYPE@ key) const
{
	auto iter = @VAR_NAME@.find(key);
	if(iter != @VAR_NAME@.end())
	{
		return iter->second;
	}
	
    return @VAR_VALUE_TYPE@{};
}

bool @OBJECT_NAME@::Set@VAR_CAMELCASE_NAME@(@VAR_KEY_TYPE@ key, @VAR_VALUE_TYPE@ val)
{
	auto iter = @VAR_NAME@.find(key);
	bool exists = iter != @VAR_NAME@.end();
	
	@VAR_NAME@[key] = val;
	
	return exists;
}

bool @OBJECT_NAME@::Remove@VAR_CAMELCASE_NAME@(@VAR_KEY_TYPE@ key)
{
	auto iter = @VAR_NAME@.find(key);
	if(iter != @VAR_NAME@.end())
	{
		@VAR_NAME@.erase(key);
		return true;
	}
	
	return false;
}

void @OBJECT_NAME@::Clear@VAR_CAMELCASE_NAME@()
{
	@VAR_NAME@.clear();
}

std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@Begin()
{
	return @VAR_NAME@.begin();
}

std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@End()
{
	return @VAR_NAME@.end();
}