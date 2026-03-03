#pragma once
//#include <type_traits>
#include "Reflection.h"
//매크로만 등록 하는 곳
// *** 매크로는 단순 문자 치환으로 띄어쓰기 줄바꿈에 매우 민감. 겨우 띄어쓰기 하나땜에 안될 수 있음




// symbol link용 함수 등록 매크로
#define REGISTER_LINK(TYPE)\
    extern "C" void Link_##TYPE();\
    extern "C" void Link_##TYPE(){}

//실제 컴포넌트 등록 
#define REGISTER_COMPONENT_DATA(TYPE) \
    static ComponentTypeInfo TYPE##_TypeInfo = { \
        #TYPE, nullptr, nullptr, [](){ return std::make_unique<TYPE>(); }, {} \
    }; \
    const char* TYPE::GetTypeName() const { return #TYPE; } \
    static bool TYPE##_Registered = [](){ \
		std::cout << "Registering component: " << #TYPE << std::endl;\
        ComponentFactory::Instance().Register(#TYPE, [](){ return std::make_unique<TYPE>(); }); \
        ComponentRegistry::Instance().Register(&TYPE##_TypeInfo); \
        return true; \
    }();

// 파생 컴포넌트 등록
#define REGISTER_COMPONENT_DERIVED_DATA(TYPE, BASE) \
    static ComponentTypeInfo TYPE##_TypeInfo = { \
        #TYPE, nullptr, #BASE, [](){ return std::make_unique<TYPE>(); }, {} \
    }; \
    const char* TYPE::GetTypeName() const { return #TYPE; } \
    static bool TYPE##_Registered = [](){ \
        std::cout << "Registering component: " << #TYPE << " (base: " << #BASE << ")" << std::endl; \
        ComponentFactory::Instance().Register(#TYPE, [](){ return std::make_unique<TYPE>(); }); \
        ComponentRegistry::Instance().Register(&TYPE##_TypeInfo); \
        return true; \
    }();

// Link함수 + Data 자동화
#define REGISTER_COMPONENT(TYPE) \
     REGISTER_COMPONENT_DATA(TYPE) \
     REGISTER_LINK(TYPE)

#define REGISTER_COMPONENT_DERIVED(TYPE, BASE) \
    REGISTER_COMPONENT_DERIVED_DATA(TYPE, BASE) \
    REGISTER_LINK(TYPE)

#define REGISTER_UI_COMPONENT(TYPE) \
    REGISTER_COMPONENT_DATA(TYPE) \
    static bool TYPE##_UIRegistered = [](){ \
        ComponentRegistry::Instance().RegisterUIType(#TYPE); \
        return true; \
    }(); \
    REGISTER_LINK(TYPE)


// private 어캐 함? Get
// float3  어캐 할 거냐
// parent
//#define REGISTER_PROPERTY(TYPE, FIELD) \
//    static bool TYPE##_##FIELD##_registered = [](){ \
//    TYPE##_TypeInfo.properties.emplace_back( \
//    std::make_unique<MemberProperty<TYPE, decltype(TYPE::FIELD)>>( \
//    #FIELD, &TYPE::FIELD \
//	)); \
//    return true; \
//    }();


#define REGISTER_PROPERTY(TYPE, NAME)\
    static bool TYPE##_##NAME##_registered = [](){\
    using RawReturn = decltype(std::declval<TYPE>().Get##NAME());\
    using ValueType = std::remove_cvref_t<RawReturn>; \
    TYPE##_TypeInfo.properties.emplace_back(\
    std::make_unique<AccessorProperty<TYPE, ValueType, true>>(\
    #NAME, &TYPE::Get##NAME, &TYPE::Set##NAME\
    )\
    );\
    return true;\
    }();

// 핸들 전용 (AssetRef로 저장/로드)
#define REGISTER_PROPERTY_HANDLE(TYPE, NAME)\
    static bool TYPE##_##NAME##_registered = [](){\
    using RawReturn = decltype(std::declval<TYPE>().Get##NAME());\
    using ValueType = std::remove_cvref_t<RawReturn>; \
    TYPE##_TypeInfo.properties.emplace_back(\
    std::make_unique<AccessorProperty<TYPE, ValueType, true>>(\
    #NAME, &TYPE::Get##NAME, &TYPE::Set##NAME\
    )\
    );\
    return true;\
    }();

// 읽기전용+저장/로드 가능(로드는 LoadSet##NAME)
#define REGISTER_PROPERTY_READONLY_LOADABLE(TYPE, NAME)\
    static bool TYPE##_##NAME##_registered = [](){\
    using RawReturn = decltype(std::declval<TYPE>().Get##NAME());\
    using ValueType = std::remove_cvref_t<RawReturn>; \
    TYPE##_TypeInfo.properties.emplace_back(\
        std::make_unique<ReadOnlyLoadableAccessorProperty<TYPE, ValueType, true>>(\
            #NAME, &TYPE::Get##NAME, &TYPE::LoadSet##NAME\
        )\
    );\
    return true;\
    }();

// 읽기전용 + 저장/로드 안 함 (에디터 수정만 막음)
#define REGISTER_PROPERTY_READONLY(TYPE, NAME) \
    static bool TYPE##_##NAME##_registered = [](){ \
    using RawReturn = decltype(std::declval<TYPE>().Get##NAME()); \
    using ValueType = std::remove_cvref_t<RawReturn>; \
    TYPE##_TypeInfo.properties.emplace_back( \
        std::make_unique<ReadOnlyAccessorProperty<TYPE, ValueType>>( \
            #NAME, &TYPE::Get##NAME \
        ) \
    ); \
    return true; \
    }();