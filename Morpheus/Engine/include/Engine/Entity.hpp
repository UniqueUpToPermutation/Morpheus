/*
*	Description: Provides a basic implementation of an entity hierarchy.
*/

#pragma once

#include <entt/entt.hpp>

#include <Engine/Components/UniquePointerComponent.hpp>

template<typename... Type>
entt::type_list<Type...> as_type_list(const entt::type_list<Type...> &);

template<typename Entity>
struct PolyStorage: entt::type_list_cat_t<
    decltype(as_type_list(std::declval<entt::Storage<Entity>>())),
    entt::type_list<
        void(entt::basic_registry<Entity> &, const Entity, const void *),
        const void *(const Entity) const,
        const Entity *() const,
        const void *() const,
        std::size_t() const,
        void(entt::basic_registry<Entity> &, const Entity *, const void *, const std::size_t)
    >
> {
    using entity_type = Entity;
    using size_type = std::size_t;

    template<typename Base>
    struct type: entt::Storage<Entity>::template type<Base> {
        static constexpr auto base = std::tuple_size_v<typename entt::poly_vtable<entt::Storage<Entity>>::type>;

        void emplace(entt::basic_registry<entity_type> &owner, const entity_type entity, const void *instance) {
            entt::poly_call<base + 0>(*this, owner, entity, instance);
        }

        const void * get(const entity_type entity) const {
            return entt::poly_call<base + 1>(*this, entity);
        }

        const entity_type * data() const {
            return entt::poly_call<base + 2>(*this);
        }

        const void * raw() const {
            return entt::poly_call<base + 3>(*this);
        }

        size_type size() const {
            return entt::poly_call<base + 4>(*this);
        }

        void insert(entt::basic_registry<Entity> &owner, const Entity *entity, const void *instance, const std::size_t length) {
            entt::poly_call<base + 5>(*this, owner, entity, instance, length);
        }
    };

    template<typename Type>
    struct members {
        static void emplace(Type &self, entt::basic_registry<entity_type> &owner, const entity_type entity, const void *instance) {
            self.emplace(owner, entity, *static_cast<const typename Type::value_type *>(instance));
        }

        static const typename Type::value_type * get(const Type &self, const entity_type entity) {
            return &self.get(entity);
        }

        static void insert(Type &self, entt::basic_registry<entity_type> &owner, const entity_type *entity, const void *instance, const size_type length) {
            const auto *value = static_cast<const typename Type::value_type *>(instance);
            self.insert(owner, entity, entity + length, value, value + length);
        }
    };

    template<typename Type>
    using impl = entt::value_list_cat_t<
        typename entt::Storage<Entity>::template impl<Type>,
        entt::value_list<
            &members<Type>::emplace,
            &members<Type>::get,
            &Type::data,
            entt::overload<const typename Type::value_type *() const ENTT_NOEXCEPT>(&Type::raw),
            &Type::size,
            &members<Type>::insert
        >
    >;
};

template<typename Entity>
struct entt::poly_storage_traits<Entity> {
    using storage_type = entt::poly<PolyStorage<Entity>>;
};

namespace Morpheus {
	class Scene;

	struct HierarchyData {
		entt::entity mParent;
		entt::entity mPrevious;
		entt::entity mNext;
		entt::entity mFirstChild;
		entt::entity mLastChild;

		HierarchyData() :
			mParent(entt::null),
			mPrevious(entt::null),
			mNext(entt::null),
			mFirstChild(entt::null),
			mLastChild(entt::null) {
		}

		HierarchyData(entt::entity parent) :
			mParent(parent),
			mPrevious(entt::null),
			mNext(entt::null),
			mFirstChild(entt::null),
			mLastChild(entt::null) {	
		}

		HierarchyData(entt::entity parent,
			entt::entity previous,
			entt::entity next) :
			mParent(parent),
			mPrevious(previous),
			mNext(next),
			mFirstChild(entt::null),
			mLastChild(entt::null) {
		}

		HierarchyData(entt::entity parent,
			entt::entity previous,
			entt::entity next,
			entt::entity firstChild,
			entt::entity lastChild) :
			mParent(parent),
			mPrevious(previous),
			mNext(next),
			mFirstChild(firstChild),
			mLastChild(lastChild) {
		}
	};

	class EntityNode {
	private:
		entt::registry* mRegistry;
		entt::entity mEntity;

		void Orphan(HierarchyData& selfData) {
			if (selfData.mParent != entt::null) {
				HierarchyData& parentData = mRegistry->get<HierarchyData>(selfData.mParent);

				if (parentData.mFirstChild == mEntity) {
					parentData.mFirstChild = selfData.mNext;
					if (selfData.mNext != entt::null) {
						HierarchyData& nextData = mRegistry->get<HierarchyData>(selfData.mNext);
						nextData.mPrevious = entt::null;
					}
				}

				if (parentData.mLastChild == mEntity) {
					parentData.mLastChild = selfData.mPrevious;
					if (selfData.mPrevious != entt::null) {
						HierarchyData& prevData = mRegistry->get<HierarchyData>(selfData.mPrevious);
						prevData.mNext = entt::null;
					}
				}
			}

			selfData.mParent = entt::null;
		}

		void DestroyInternal() {
			HierarchyData& data = mRegistry->get<HierarchyData>(mEntity);

			for (EntityNode child = GetFirstChild(); child.IsValid(); child = child.GetNext()) {
				child.DestroyInternal();
			}

			mRegistry->destroy(mEntity);
			mEntity = entt::null;
		}

	public:
		EntityNode(entt::registry* registry, entt::entity e) :
			mRegistry(registry),
			mEntity(e) {
		}

		EntityNode() :
			mRegistry(nullptr),
			mEntity(entt::null) {
		}

		inline bool IsValid() const {
			return mEntity != entt::null;
		}

		inline bool IsInvalid() const {
			return mEntity == entt::null;
		}

		static EntityNode Invalid() {
			return EntityNode();
		}

		inline HierarchyData GetAdjacencyData() {
			return mRegistry->get<HierarchyData>(mEntity);
		}

		inline EntityNode GetParent() {
			auto& data = mRegistry->get<HierarchyData>(mEntity);
			return EntityNode(mRegistry, data.mParent);
		}

		inline EntityNode GetFirstChild() {
			auto& data = mRegistry->get<HierarchyData>(mEntity);
			return EntityNode(mRegistry, data.mFirstChild);
		}

		inline EntityNode GetLastChild() {
			auto& data = mRegistry->get<HierarchyData>(mEntity);
			return EntityNode(mRegistry, data.mLastChild);
		}

		inline EntityNode GetNext() {
			auto& data = mRegistry->get<HierarchyData>(mEntity);
			return EntityNode(mRegistry, data.mNext);
		}

		inline EntityNode GetPrevious() {
			auto& data = mRegistry->get<HierarchyData>(mEntity);
			return EntityNode(mRegistry, data.mPrevious);
		}

		template <typename T>
		inline bool Has() {
			return mRegistry->has<T>(mEntity);
		}

		void AddChild(EntityNode e) {
			auto& otherData = mRegistry->get<HierarchyData>(e.mEntity);

			if (otherData.mParent != entt::null) {
				// Make this node into an orphan
				e.Orphan(otherData);
			}

			auto& selfData = mRegistry->get<HierarchyData>(mEntity);
			
			// Add to the end of linked child list
			if (selfData.mLastChild != entt::null) {
				auto& prevLastData = mRegistry->get<HierarchyData>(selfData.mLastChild);

				prevLastData.mNext = e.mEntity;
				otherData.mPrevious = selfData.mLastChild;
				selfData.mLastChild = e.mEntity;

			} else {
				selfData.mFirstChild = e.mEntity;
				selfData.mLastChild = e.mEntity;
			}

			otherData.mParent = mEntity;
		}

		EntityNode CreateChild() {
			entt::entity e = mRegistry->create();
			mRegistry->emplace<HierarchyData>(e);
			EntityNode node(mRegistry, e);
			AddChild(node);
			return node;
		}

		EntityNode CreateChild(entt::entity e) {
			mRegistry->emplace<HierarchyData>(e);
			EntityNode node(mRegistry, e);
			AddChild(node);
			return node;
		}

		inline entt::entity GetEntity() const {
			return mEntity;
		}

		inline entt::registry* GetRegistry() {
			return mRegistry;
		}

		inline void Orphan() {
			Orphan(mRegistry->get<HierarchyData>(mEntity));
		}

		inline void SetParent(EntityNode& e) {
			e.AddChild(*this);
		}

		void Destroy() {
			Orphan();
			DestroyInternal();
		}

		template <typename T>
		inline decltype(auto) Get() {
			return mRegistry->template get<T>(mEntity);
		}

		template <typename T>
		inline decltype(auto) TryGet() {
			return mRegistry->template try_get<T>(mEntity);
		}

		template <typename T, typename... Args>
		inline decltype(auto) Add(Args &&... args) {
			return mRegistry->template emplace<T, Args...>(mEntity, std::forward<Args>(args)...);
		}

		template<typename Component, typename... Func>
    	inline decltype(auto) Patch(Func &&... func) {
			return mRegistry->template patch<Component, Func...>(mEntity, std::forward<Func>(func)...);
		}

		template<typename Component, typename... Args>
		inline decltype(auto) Replace(Args &&... args) {
			if constexpr (IsUniquePointerComponent<Component>()) {
				static_assert("Cannot used replace with unique pointer components!");
			} else {
				return mRegistry->template replace<Component, Args...>(mEntity, std::forward<Args>(args)...);
			}
		}

		template <typename Component>
		inline void Remove() {
			mRegistry->template remove<Component>(mEntity);
		}

		template <typename Component, typename... Args>
		inline decltype(auto) AddOrReplace(Args &&... args) {
			if constexpr (IsUniquePointerComponent<Component>()) {
				static_assert("Cannot used replace with unique pointer components!");
			} else {
				return mRegistry->template emplace_or_replace<Component, Args...>(mEntity, std::forward<Args>(args)...);
			}
		}
	};
}