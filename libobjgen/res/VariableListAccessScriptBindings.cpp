.Func<@VAR_TYPE@ (@OBJECT_NAME@::*)(size_t) const>(
    "Get@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@)
.Func<bool (@OBJECT_NAME@::*)(@VAR_ARG_TYPE@)>(
    "Append@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Append@VAR_CAMELCASE_NAME@)
.Func<bool (@OBJECT_NAME@::*)(@VAR_ARG_TYPE@)>(
    "Prepend@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Prepend@VAR_CAMELCASE_NAME@)
.Func<bool (@OBJECT_NAME@::*)(size_t, @VAR_ARG_TYPE@)>(
    "Insert@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Insert@VAR_CAMELCASE_NAME@)
.Func<bool (@OBJECT_NAME@::*)(size_t)>(
    "Remove@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Remove@VAR_CAMELCASE_NAME@)
.Func<void (@OBJECT_NAME@::*)()>(
    "Clear@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Clear@VAR_CAMELCASE_NAME@)
.Func<size_t (@OBJECT_NAME@::*)() const>(
    "@VAR_CAMELCASE_NAME@Count", &@OBJECT_NAME@::@VAR_CAMELCASE_NAME@Count)