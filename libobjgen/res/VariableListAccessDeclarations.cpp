@VAR_TYPE@ Get@VAR_CAMELCASE_NAME@(size_t index) const;
void Append@VAR_CAMELCASE_NAME@(@VAR_TYPE@ val);
void Prepend@VAR_CAMELCASE_NAME@(@VAR_TYPE@ val);
bool Insert@VAR_CAMELCASE_NAME@(size_t index, @VAR_TYPE@ val);
bool Remove@VAR_CAMELCASE_NAME@(size_t index);
void Clear@VAR_CAMELCASE_NAME@();
std::list<@VAR_TYPE@>::iterator @VAR_CAMELCASE_NAME@Begin();
std::list<@VAR_TYPE@>::iterator @VAR_CAMELCASE_NAME@End();