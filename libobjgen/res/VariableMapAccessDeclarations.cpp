@VAR_VALUE_TYPE@ Get@VAR_CAMELCASE_NAME@(@VAR_KEY_TYPE@ key) const;
bool Set@VAR_CAMELCASE_NAME@(@VAR_KEY_TYPE@ key, @VAR_VALUE_TYPE@ val);
bool Remove@VAR_CAMELCASE_NAME@(@VAR_KEY_TYPE@ key);
void Clear@VAR_CAMELCASE_NAME@();
std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::iterator @VAR_CAMELCASE_NAME@Begin();
std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::iterator @VAR_CAMELCASE_NAME@End();