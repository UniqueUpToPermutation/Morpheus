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

		static void Orphan(entt::registry& registry, entt::entity ent) {
			HierarchyData& data = registry.get<HierarchyData>(ent);

			if (data.mParent != entt::null) {
				HierarchyData& parentData = registry.get<HierarchyData>(data.mParent);

				if (parentData.mFirstChild == ent) {
					parentData.mFirstChild = data.mNext;
					if (data.mNext != entt::null) {
						HierarchyData& nextData = registry.get<HierarchyData>(data.mNext);
						nextData.mPrevious = entt::null;
					}
				}

				if (parentData.mLastChild == ent) {
					parentData.mLastChild = data.mPrevious;
					if (data.mPrevious != entt::null) {
						HierarchyData& prevData = registry.get<HierarchyData>(data.mPrevious);
						prevData.mNext = entt::null;
					}
				}
			}

			data.mParent = entt::null;
		}

		static void AddChild(entt::registry& registry, entt::entity parent, entt::entity newChild) {
			auto& childData = registry.get<HierarchyData>(newChild);

			if (childData.mParent != entt::null) {
				// Make this node into an orphan
				Orphan(registry, newChild);
			}

			auto& selfData = registry.get<HierarchyData>(parent);
			
			// Add to the end of linked child list
			if (selfData.mLastChild != entt::null) {
				auto& prevLastData = registry.get<HierarchyData>(selfData.mLastChild);

				prevLastData.mNext = newChild;
				childData.mPrevious = selfData.mLastChild;
				selfData.mLastChild = newChild;

			} else {
				selfData.mFirstChild = newChild;
				selfData.mLastChild = newChild;
			}

			childData.mParent = parent;
		}
	};
}