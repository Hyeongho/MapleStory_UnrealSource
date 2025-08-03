#pragma once

#include "TypeID.h"

// Ŭ���� ����ο� ���̼���
#define DECLARE_CASTABLE(ClassType)                                \
public:                                                            \
    static size_t StaticTypeID() { return TypeID<ClassType>::Value(); } \
    virtual size_t GetTypeID() const override { return StaticTypeID(); }