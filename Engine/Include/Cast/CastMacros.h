#pragma once

#include "TypeID.h"

// 클래스 선언부에 붙이세요
#define DECLARE_CASTABLE(ClassType)                                \
public:                                                            \
    static size_t StaticTypeID() { return TypeID<ClassType>::Value(); } \
    virtual size_t GetTypeID() const override { return StaticTypeID(); }