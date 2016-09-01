/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, development version     *
*                (c) 2006-2016 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This library is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This library is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this library; if not, write to the Free Software Foundation,     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.          *
*******************************************************************************
*                               SOFA :: Plugins                               *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef __PYTHON_FACTORY_H
#define __PYTHON_FACTORY_H

#include "PythonMacros.h"
#include <SofaPython/config.h>


#include <SofaPython/Binding_Base.h>
#include <SofaPython/Binding_BaseObject.h>
#include <SofaPython/Binding_BaseLoader.h>
#include <SofaPython/Binding_Topology.h>
#include <SofaPython/Binding_BaseMeshTopology.h>
#include <SofaPython/Binding_VisualModel.h>
#include <SofaPython/Binding_BaseState.h>
#include <SofaPython/Binding_BaseMechanicalState.h>
#include <SofaPython/Binding_BaseMapping.h>
#include <SofaPython/Binding_DataEngine.h>
#include <SofaPython/Binding_BaseContext.h>
#include <SofaPython/Binding_Data.h>

#include <boost/type_traits/is_base_of.hpp>


namespace sofa
{


// shortcut for SP_ADD_CLASS with fixed sofa module
#define SP_ADD_CLASS_IN_SOFAMODULE(C) SP_ADD_CLASS( PythonFactory::s_sofaPythonModule, C )


/// This is the macro to call to bind a new type inherited from sofa::core::objectmodel::Base
#define SP_ADD_CLASS_IN_FACTORY( PYTHONNAME, CPPCLASSNAME ) {\
    SP_ADD_CLASS_IN_SOFAMODULE( PYTHONNAME )  \
    PythonFactory::add<CPPCLASSNAME>( &SP_SOFAPYTYPEOBJECT(PYTHONNAME) ); \
    }


/// @internal To convert a sofa::core::objectmodel::Base
/// or a sofa::core::objectmodel::BaseData
/// in its corresponding pyObject
// retourne automatiquement le type Python de plus haut niveau possible,
// en fonction du type de l'objet Cpp (spécifiquement bindé)
// afin de permettre l'utilisation de fonctions des sous-classes de Base
class SOFA_SOFAPYTHON_API PythonFactory
{

public:
    static PyObject *s_sofaPythonModule;

protected:

    /// a undefined bound type
    /// nothing can be casted in it
    struct BasePythonBoundType
    {
        BasePythonBoundType(PyTypeObject*pyTypeObject):pyTypeObject(pyTypeObject){}
        virtual bool canCast(sofa::core::objectmodel::Base*) const { return false; }
        virtual bool canCast(sofa::core::objectmodel::BaseData*) const { return false; }
        PyTypeObject* pyTypeObject;
    };

    /// a know bound type (built when calling 'add')
    /// cast is checked by dynamic_cast (this is similar to the previous implementation, but could be improved)
    template <class T>
    struct PythonBoundType : public BasePythonBoundType
    {
        PythonBoundType(PyTypeObject*pyTypeObject):BasePythonBoundType(pyTypeObject){}
        virtual bool canCast(sofa::core::objectmodel::Base* obj) const { return dynamic_cast<T*>(obj); }
        virtual bool canCast(sofa::core::objectmodel::BaseData* data) const { return dynamic_cast<T*>(data); }
    };

    /// a list of Abstract classes that can be cheaply deduced from Base* (by static_cast)
    /// this limits checking the right cast on a limited number of types
    /// Note this list is built from actual needs, but can be easily extended to any types that Base* can be statically casted from.
    enum{Base=0,BaseObject,BaseLoader,Topology,BaseMeshTopology,VisualModel,BaseState,BaseMechanicalState,BaseMapping,DataEngine,BaseContext,NB_LISTS};
    typedef std::list< BasePythonBoundType* > PythonBoundTypes;
    /// a list of types for each sub-classes (prefiltering types not to have to check casting with any of them)
    static PythonBoundTypes s_boundComponents[NB_LISTS];
    /// a list of Data types
    static PythonBoundTypes s_boundData;


    /// check for the corresponding type in the given list
    static PyObject* toPython( const PythonBoundTypes& list, sofa::core::objectmodel::Base* obj, PyTypeObject* pyTypeObject  )
    {
//        std::cerr<<"toPython "<<obj->getClassName()<<std::endl;

        for(PythonBoundTypes::const_reverse_iterator it=list.rbegin(),itend=list.rend();it!=itend;++it)
            if( (*it)->canCast( obj ) ) return BuildPySPtr<sofa::core::objectmodel::Base>(obj,(*it)->pyTypeObject);

        return BuildPySPtr<sofa::core::objectmodel::Base>(obj,pyTypeObject);
    }


public:

    /// add a new sofa::core::objectmodel::Base-inherited type
    /// or a sofa::core::objectmodel::BaseData-inherited type
    /// with it corresponding pyTypeObject to the Factory
    template<class T>
    static void add( PyTypeObject* pyTypeObject )
    {
//        std::cerr<<"ADD "<<T::template className<T>()<<std::endl;

        PythonBoundType<T>* t = new PythonBoundType<T>(pyTypeObject);

        if( boost::is_base_of<sofa::core::objectmodel::BaseObject,T>::value )
        {
            if( boost::is_base_of<sofa::core::loader::BaseLoader, T>::value )
                return s_boundComponents[BaseLoader].push_back( t );

            if( boost::is_base_of<sofa::core::topology::Topology, T>::value )
            {
                if( boost::is_base_of<sofa::core::topology::BaseMeshTopology, T>::value )
                    return s_boundComponents[BaseMeshTopology].push_back( t );
                return s_boundComponents[Topology].push_back( t );
            }

            if( boost::is_base_of<sofa::core::visual::VisualModel, T>::value )
                return s_boundComponents[VisualModel].push_back( t );

            if( boost::is_base_of<sofa::core::BaseState, T>::value )
            {
                if( boost::is_base_of<sofa::core::behavior::BaseMechanicalState, T>::value )
                    return s_boundComponents[BaseMechanicalState].push_back( t );

                return s_boundComponents[BaseState].push_back( t );
            }

            if( boost::is_base_of<sofa::core::BaseMapping, T>::value )
                return s_boundComponents[BaseMapping].push_back( t );

            if( boost::is_base_of<sofa::core::DataEngine, T>::value )
                return s_boundComponents[DataEngine].push_back( t );

            return s_boundComponents[BaseObject].push_back( t );
        }
        else if( boost::is_base_of<sofa::core::objectmodel::BaseContext, T>::value )
            return s_boundComponents[BaseContext].push_back( t );
        else if( boost::is_base_of<sofa::core::objectmodel::Base, T>::value )
            return s_boundComponents[Base].push_back( t );

        return s_boundData.push_back( t );
    }



    /// to convert a sofa::core::objectmodel::Base-inherited object to its corresponding pyObject
    /// could be improve for known types, but since it is only setup...
    static PyObject* toPython(sofa::core::objectmodel::Base* obj)
    {
//        std::cerr<<"toPython0 "<<obj->getClassName()<<std::endl;

        if( obj->toBaseObject() )
        {
            if( obj->toBaseLoader() ) return toPython( obj->toBaseLoader() );

            if( obj->toTopology() )
            {
                if( obj->toBaseMeshTopology() ) return toPython( obj->toBaseMeshTopology() );
                return toPython( obj->toTopology() );
            }

            if( obj->toVisualModel()) return toPython( obj->toVisualModel() );

            if( obj->toBaseState() )
            {
                if (obj->toBaseMechanicalState()) return toPython( obj->toBaseMechanicalState() );

                return toPython( obj->toBaseState() );
            }

            if (obj->toBaseMapping()) return toPython( obj->toBaseMapping() );

            if (obj->toDataEngine()) return toPython( obj->toDataEngine() );

            return toPython( obj->toBaseObject() );
        }
        else if( obj->toBaseContext() ) return toPython( obj->toBaseContext() );

        return toPython( s_boundComponents[Base], obj, &SP_SOFAPYTYPEOBJECT(Base) );
    }


    /// to convert a BaseObject-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::objectmodel::BaseObject* obj)
    {
        if( obj->toBaseLoader() ) return toPython( obj->toBaseLoader() );

        if( obj->toTopology() )
        {
            if( obj->toBaseMeshTopology() ) return toPython( obj->toBaseMeshTopology() );
            return toPython( obj->toTopology() );
        }

        if( obj->toVisualModel()) return toPython( obj->toVisualModel() );

        if( obj->toBaseState() )
        {
            if (obj->toBaseMechanicalState()) return toPython( obj->toBaseMechanicalState() );

            return toPython( obj->toBaseState() );
        }

        if (obj->toBaseMapping()) return toPython( obj->toBaseMapping() );

        if (obj->toDataEngine()) return toPython( obj->toDataEngine() );

        return toPython( s_boundComponents[BaseObject], obj, &SP_SOFAPYTYPEOBJECT(BaseObject) );
    }

    /// to convert a BaseContext-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::objectmodel::BaseContext* obj)
    {
        return toPython( s_boundComponents[BaseContext], obj, &SP_SOFAPYTYPEOBJECT(BaseContext) );
    }

    /// to convert a BaseLoader-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::loader::BaseLoader* obj)
    {
        return toPython( s_boundComponents[BaseLoader], obj, &SP_SOFAPYTYPEOBJECT(BaseLoader) );
    }

    /// to convert a Topology-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::topology::Topology* obj)
    {
        if( obj->toBaseMeshTopology() ) return toPython( obj->toBaseMeshTopology() );
        return toPython( s_boundComponents[Topology], obj, &SP_SOFAPYTYPEOBJECT(Topology) );
    }

    /// to convert a BaseMeshTopology-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::topology::BaseMeshTopology* obj)
    {
        return toPython( s_boundComponents[BaseMeshTopology], obj, &SP_SOFAPYTYPEOBJECT(BaseMeshTopology) );
    }

    /// to convert a VisualModel-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::visual::VisualModel* obj)
    {
        return toPython( s_boundComponents[VisualModel], obj, &SP_SOFAPYTYPEOBJECT(VisualModel) );
    }

    /// to convert a BaseState-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::BaseState* obj)
    {
        if (obj->toBaseMechanicalState()) return toPython( obj->toBaseMechanicalState() );

        return toPython( s_boundComponents[BaseState], obj, &SP_SOFAPYTYPEOBJECT(BaseState) );
    }

    /// to convert a BaseMechanicalState-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::behavior::BaseMechanicalState* obj)
    {
        return toPython( s_boundComponents[BaseMechanicalState], obj, &SP_SOFAPYTYPEOBJECT(BaseMechanicalState) );
    }

    /// to convert a BaseMapping-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::BaseMapping* obj)
    {
        return toPython( s_boundComponents[BaseMapping], obj, &SP_SOFAPYTYPEOBJECT(BaseMapping) );
    }

    /// to convert a DataEngine-inherited object to its corresponding pyObject
    static PyObject* toPython(sofa::core::DataEngine* obj)
    {
        return toPython( s_boundComponents[DataEngine], obj, &SP_SOFAPYTYPEOBJECT(DataEngine) );
    }


    /// to convert a sofa::core::objectmodel::BaseData to its corresponding pyObject
    /// returns NULL if the data does not correpond to any special type
    static PyObject* toPython(sofa::core::objectmodel::BaseData* data)
    {
        for(PythonBoundTypes::const_reverse_iterator it=s_boundData.rbegin(),itend=s_boundData.rend();it!=itend;++it)
        {
            if( (*it)->canCast( data ) )
                return BuildPyPtr<sofa::core::objectmodel::BaseData>(data,(*it)->pyTypeObject,false);
        }

//        return BuildPyPtr<sofa::core::objectmodel::BaseData>(data,&SP_SOFAPYTYPEOBJECT(Data),false);
        return NULL;
    }


private:

    // singleton
    PythonFactory();
    PythonFactory(const PythonFactory&);


};



} // namespace sofa

#endif