#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <typeinfo>
#include "MathHelper.h"
#include "Serializer.h"
#include "Component.h"
#include "json.hpp"

using namespace std;
using namespace MathUtils;
	
	/// 모든 데이터 type에 대해 어떻게 역직/직렬화 될지에 대해 정해줘야 함.
	

	class Property {
	public:
		explicit Property(const char* name, bool serializable = true) : m_Name(name), m_Serializable(serializable) {}
		virtual ~Property() = default;

		const string& GetName() const { return m_Name; }
		bool   IsSerializable() const { return m_Serializable; }

		//virtual void DrawEditor(Component* c) const;
		virtual const std::type_info& GetTypeInfo() const = 0;
		virtual void GetValue(Component* c, void* outValue) const = 0;
		virtual void SetValue(Component* c, const void* inValue) const = 0;
		virtual void Serialize(Component* c, nlohmann::json& j) const = 0;
		virtual void Deserialize(Component* c, const nlohmann::json& j) const = 0;

	private:
		std::string m_Name;
		bool m_Serializable = true;
	};

	//////
	// 멤버 직접 접근 public 멤버만 가능 (아마 쓸일 없음)
	//template<typename T, typename Field>
	//class MemberProperty : public Property {
	//public:
	//	using MemberPtr = Field T::*;
	//
	//	MemberProperty(const char* name, MemberPtr member)
	//		: Property(name), m_Member(member) {
	//	}
	//
	//	//void DrawEditor(Component* c) const override {
	//	//	T* obj = static_cast<T*>(c);
	//	//	Editor::Draw(GetName().c_str(), obj->*m_Member);
	//	//}
	//
	//	void Serialize(Component* c, nlohmann::json& j) const override
	//	{
	//		T* obj = static_cast<T*>(c);
	//		j[GetName()] = obj->*m_Member;
	//	}
	//
	//	void Deserialize(Component* c, const nlohmann::json& j) const override
	//	{
	//		if (!j.contains(GetName())) return;
	//		T* obj = static_cast<T*>(c);
	//		obj->*m_Member = j[GetName()].get<Field>();
	//	}
	//
	//private:
	//	MemberPtr m_Member;
	//};


	////
	//Getter, Setter 사용 (serializable 템플릿 bool)
	template<typename T, typename Value, bool serializable = true>
	class AccessorProperty : public Property {
	public:
		using Getter = const Value& (T::*)() const;
		using Setter = void (T::*)(const Value&);

		template<typename G, typename S>
		AccessorProperty(const char* name, G g, S s)
			: Property(name, serializable)
			, m_Get(reinterpret_cast<Getter>(g))
			, m_Set(reinterpret_cast<Setter>(s))
		{
		}

		const std::type_info& GetTypeInfo() const override {
			return typeid(Value);
		}

		void GetValue(Component* c, void* outValue) const override {
			T* obj = static_cast<T*>(c);
			*static_cast<Value*>(outValue) = (obj->*m_Get)();
		}

		void SetValue(Component* c, const void* inValue) const override {
			T* obj = static_cast<T*>(c);
			(obj->*m_Set)(*static_cast<const Value*>(inValue));
		}

		void Serialize(Component* c, nlohmann::json& j) const override {
			if constexpr (serializable) {
				T* obj = static_cast<T*>(c);
				Serializer<Value>::ToJson(j[GetName()], (obj->*m_Get)());
			}
		}

		void Deserialize(Component* c, const nlohmann::json& j) const override {
			if (!j.contains(GetName())) return;
			if constexpr (serializable) {
				T* obj = static_cast<T*>(c);
				Value v{};
				Serializer<Value>::FromJson(j[GetName()], v);
				(obj->*m_Set)(v);
			}
		}

	private:
		Getter m_Get;
		Setter m_Set;
	};
	
	// ReadOnly(에디터 Set 막음) + Load는 가능(LoadSetter 호출)
	template<typename T, typename Value, bool serializable = true>
	class ReadOnlyLoadableAccessorProperty : public Property {
	public:
		using Getter = const Value& (T::*)() const;
		using LoadSetter = void (T::*)(const Value&);

		template<typename G, typename S>
		ReadOnlyLoadableAccessorProperty(const char* name, G g, S loadSetter)
			: Property(name, serializable)
			, m_Get(reinterpret_cast<Getter>(g))
			, m_LoadSet(reinterpret_cast<LoadSetter>(loadSetter))
		{
		}

		const std::type_info& GetTypeInfo() const override {
			return typeid(Value);
		}

		void GetValue(Component* c, void* outValue) const override {
			T* obj = static_cast<T*>(c);
			*static_cast<Value*>(outValue) = (obj->*m_Get)();
		}

		// 에디터/외부 수정 차단
		void SetValue(Component* c, const void* inValue) const override {

		}

		void Serialize(Component* c, nlohmann::json& j) const override {
			if constexpr (serializable) {
				T* obj = static_cast<T*>(c);
				Serializer<Value>::ToJson(j[GetName()], (obj->*m_Get)());
			}
		}

		void Deserialize(Component* c, const nlohmann::json& j) const override {
			if constexpr (serializable) {
				if (!j.contains(GetName())) return;
				T* obj = static_cast<T*>(c);
				Value v{};
				Serializer<Value>::FromJson(j[GetName()], v);
				(obj->*m_LoadSet)(v); // 로드에서만 내부 갱신
			}
		}

	private:
		Getter	   m_Get;
		LoadSetter m_LoadSet;
	};

	template<typename T, typename Value>
	class ReadOnlyAccessorProperty : public Property {
	public:
		using Getter = const Value& (T::*)() const;

		template<typename G>
		ReadOnlyAccessorProperty(const char* name, G g)
			: Property(name, false)
			, m_Get(reinterpret_cast<Getter>(g))
		{
		}

		const std::type_info& GetTypeInfo() const override { return typeid(Value); }

		void GetValue(Component* c, void* outValue) const override {
			T* obj = static_cast<T*>(c);
			*static_cast<Value*>(outValue) = (obj->*m_Get)();
		}

		void SetValue(Component*, const void*) const override {} // 수정 금지
		void Serialize(Component*, nlohmann::json&) const override {} // 저장 안 함
		void Deserialize(Component*, const nlohmann::json&) const override {} // 로드 안 함

	private:
		Getter m_Get;
	};

	using ComponentCreateFunc = function<unique_ptr<Component>()>;

	struct ComponentTypeInfo {
		string name;  // 컴포넌트 이름 TransformComponent
		ComponentTypeInfo* parent = nullptr;	 // 상속된 경우 알아서 들고 오도록
		const char* parentName = nullptr;		 // 지연 연결용 이름 추가
		ComponentCreateFunc factory;
		vector<unique_ptr<Property>> properties; // 컴포넌트가 가지고 있는 Property(변수 등)
	};

	class ComponentRegistry {
	public:
		static ComponentRegistry& Instance();

		void Register(ComponentTypeInfo* info);
		void RegisterUIType(const string& name) { m_UITypes.insert(name); }
		bool IsUIType(const string& name) const { return m_UITypes.find(name) != m_UITypes.end(); }
		vector<string> GetTypeNames() const; // 등론된 이름 전체 return
		ComponentTypeInfo* Find(const string& name) const {
			auto it = m_Types.find(name);
			return (it != m_Types.end()) ? it->second : nullptr;
		}

		// 핵심: parentName 기반 지연 연결 + base->derived 순서로 property 합치기
		std::vector<const Property*> CollectProperties(const ComponentTypeInfo* type) const
		{
			std::vector<const Property*> out;
			if (!type) return out;

			ComponentTypeInfo* t = const_cast<ComponentTypeInfo*>(type);

			// parent 포인터가 비어있고 parentName이 있으면 지금 resolve 시도
			if (!t->parent && t->parentName) {
				t->parent = Find(t->parentName);
			}

			if (!t->parent && t->parentName) {
				auto* p = Find(t->parentName);
				std::cout << "[ResolveParent] child=" << t->name
					<< " parentName=" << t->parentName
					<< " found=" << p << std::endl;
				t->parent = p;
			}


			vector<const ComponentTypeInfo*> chain;
			for (auto* cur = t; cur != nullptr; cur = cur->parent) {
				chain.push_back(cur);

				// 체인 중간도 지연 연결 보정 (부모가 아직 안 물려있을 수도 있어서)
				auto* mutableCur = const_cast<ComponentTypeInfo*>(cur);
				if (!mutableCur->parent && mutableCur->parentName) {
					mutableCur->parent = Find(mutableCur->parentName);
				}
			}

			for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
				const auto* info = *it;
				for (const auto& p : info->properties)
					out.push_back(p.get());
			}

			return out;
		}

		void Check() {
			//개발 확인용
			for (const auto& [name, typeInfo] : m_Types) {
				std::cout << "Component: " << name << std::endl;

				if (!typeInfo) {
					std::cout << "  (TypeInfo is null)" << std::endl;
					continue;
				}

				auto props = CollectProperties(typeInfo);
				for (const auto* prop : props) {
					std::cout << "    Property: " << prop->GetName() << std::endl;
				}
			}
		}
	private:
		unordered_map<string, ComponentTypeInfo*> m_Types; //이름 : 컴포넌트
		unordered_set<string> m_UITypes;
	};
	// 게임에서 정의된 컴포넌트 들과 프로퍼티 (사전) -> Component List

	
