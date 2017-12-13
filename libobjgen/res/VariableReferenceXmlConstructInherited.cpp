const char *szObjectName = pRefChildNode->Attribute("name");
libcomp::String objectName;

if(nullptr != szObjectName)
{
    objectName = szObjectName;
}

if(objectName.IsEmpty())
{
    ref = @CONSTRUCT_VALUE@;
}
else
{
    ref = @REF_TYPE@::InheritedConstruction(objectName);
}
