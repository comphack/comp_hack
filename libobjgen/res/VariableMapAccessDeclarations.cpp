@VAR_VALUE_TYPE@ Get@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key);
bool @VAR_CAMELCASE_NAME@KeyExists(@VAR_KEY_ARG_TYPE@ key) const;
bool Set@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key, @VAR_VALUE_ARG_TYPE@ val);
bool Remove@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key);
void Clear@VAR_CAMELCASE_NAME@();
size_t @VAR_CAMELCASE_NAME@Count() const;
std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::const_iterator @VAR_CAMELCASE_NAME@Begin() const;
std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::const_iterator @VAR_CAMELCASE_NAME@End() const;