@VAR_TYPE@ Get@VAR_CAMELCASE_NAME@(size_t index) const;
bool Append@VAR_CAMELCASE_NAME@(@VAR_TYPE@ val);
bool Prepend@VAR_CAMELCASE_NAME@(@VAR_TYPE@ val);
bool Insert@VAR_CAMELCASE_NAME@(size_t index, @VAR_TYPE@ val);
bool Remove@VAR_CAMELCASE_NAME@(size_t index);
void Clear@VAR_CAMELCASE_NAME@();
size_t @VAR_CAMELCASE_NAME@Count() const;
std::list<@VAR_TYPE@>::const_iterator @VAR_CAMELCASE_NAME@Begin() const;
std::list<@VAR_TYPE@>::const_iterator @VAR_CAMELCASE_NAME@End() const;