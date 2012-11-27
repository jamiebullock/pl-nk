/*
 -------------------------------------------------------------------------------
 This file is part of the Plink, Plonk, Plank libraries
  by Martin Robinson
 
 http://code.google.com/p/pl-nk/
 
 Copyright University of the West of England, Bristol 2011-12
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of University of the West of England, Bristol nor 
   the names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL UNIVERSITY OF THE WEST OF ENGLAND, BRISTOL BE 
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 
 This software makes use of third party libraries. For more information see:
 doc/license.txt included in the distribution.
 -------------------------------------------------------------------------------
 */

#ifndef PLONK_VARIABLEINTERNAL_H
#define PLONK_VARIABLEINTERNAL_H

#include "plonk_VariableForwardDeclarations.h"
#include "../../core/plonk_Sender.h"


template<class Type>
class VariableInternalBase : public SenderInternal< Variable<Type> >
{
public:
    ~VariableInternalBase() { }
    virtual const Type getValue() const = 0;
    virtual const Type nextValue() = 0;
    virtual void setValue (Type const& newValue) = 0;
};

//template<class Type>
//class VariableInternalBase<Type&> : public SmartPointer
//{
//public:
//    ~VariableInternalBase() { }
//    virtual Type& getValue() = 0;
//    virtual void setValue (Type& newValue) = 0;
//};



//------------------------------------------------------------------------------


template<class Type>
class VariableInternal : public VariableInternalBase<Type>
{
public:    
    typedef Variable<Type> Container; 
    
    inline VariableInternal() throw()
    :   value (0)
    {
    }
    
    inline ~VariableInternal()
    {
    }
    
//    template<class OtherType>
//    VariableInternal (OtherType const& initValue) throw()
//    :   value (initValue)
//    {
//    }

    inline VariableInternal (Type const& initValue) throw()
    :   value (initValue)
    {
    }

    inline const Type getValue() const throw()
    {
        return value;
    }
    
    inline const Type nextValue() throw()
    {        
        return value;
    }
    
    inline void setValue (Type const& newValue) throw()
    {
        if (value != newValue)
        {
            value = newValue;
            this->update (Dynamic::getNull());
        }
    }
    
    inline void swapValues (Type& other) throw()
    {
        Type temp = other;
        other = value;
        value = temp;
    }
    
private:
    Type value;
};


template<class Type>
class VariableInternal<Type&> : public SmartPointer // VariableInternalBase<Type&>
{
public:    
    typedef Variable<Type&> Container; 
        
//    VariableInternal (Type const& initValue) throw()
//    {
//        value.setValue (initValue.getValue());
//    }
    
    template<class ValueType>
    VariableInternal (ValueType const& initValue) throw()
    {
        setValue (initValue);
    }
    
    inline Type& getValue() throw()
    {
        return value;
    }
        
    template<class ValueType>
    inline void setValue (ValueType const& newValue) throw()
    {
        value = newValue;
    }
    
    inline void swapValues (Type& other) throw()
    {
        value.swapWith (other);
    }
    
private:
    Type value;
};


#endif // PLONK_VARIABLEINTERNAL_H