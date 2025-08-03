#pragma once

class RefCountBase
{
public:
    RefCountBase() : StrongCount(1), WeakCount(0) {}

    void AddRef() 
    { 
        StrongCount++;
    }

    void ReleaseRef() 
    { 
        StrongCount--;
    }

    void AddWeak()
    { 
        WeakCount++;
    }

    void ReleaseWeak() 
    { 
        WeakCount--;
    }

    int GetStrongCount() const 
    { 
        return StrongCount; 
    }

    int GetWeakCount() const 
    { 
        return WeakCount; 
    }

private:
    int StrongCount;
    int WeakCount;
};