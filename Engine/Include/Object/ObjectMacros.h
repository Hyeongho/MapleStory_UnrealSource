#pragma once

#define UOBJ_WIDEN2(x)  L ## x
#define UOBJ_WIDEN(x)   UOBJ_WIDEN2(x)
#define UOBJ_WSTR(x)    UOBJ_WIDEN(#x)

class UClass;

#define DECLARE_CLASS_ROOT(TClass)                                \
public:                                                           \
    static UClass* StaticClass()                                  \
    {                                                             \
        static UClass s_ClassInfo(UOBJ_WSTR(TClass), nullptr);   \
        return &s_ClassInfo;                                      \
    }                                                             \
    virtual UClass* GetClass() const                              \
    {                                                             \
        return TClass::StaticClass();                             \
    }                                                             \
private:

#define DECLARE_CLASS(TClass, TSuperClass)                                 \
public:                                                                    \
    using Super = TSuperClass;                                             \
    static UClass* StaticClass()                                           \
    {                                                                      \
        static UClass s_ClassInfo(UOBJ_WSTR(TClass),                      \
                                  TSuperClass::StaticClass());             \
        return &s_ClassInfo;                                               \
    }                                                                      \
    virtual UClass* GetClass() const override                              \
    {                                                                      \
        return TClass::StaticClass();                                      \
    }                                                                      \
private:

#define UCLASS(...)
#define UPROPERTY(...)
#define UFUNCTION(...)