bool @OBJECT_NAME@::Validate@VAR_CAMELCASE_NAME@Entry(@VAR_KEY_TYPE@ key, @VAR_VALUE_TYPE@ val)
{
    bool keyValid = (@KEY_VALIDATION_CODE@);
    
    bool valueValid = (@VALUE_VALIDATION_CODE@);
    
    return keyValid && valueValid;
}