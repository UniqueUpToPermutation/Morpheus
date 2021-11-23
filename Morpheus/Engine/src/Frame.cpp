#include <Engine/Frame.hpp>
#include <Engine/Components/ResourceComponent.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Reflection.hpp>

#include <cereal/types/string.hpp>
#include <cereal/archives/portable_binary.hpp>

namespace Morpheus {
	const std::unordered_map<entt::entity, ArchiveBlobPointer>&
		Frame::GetResourceTable() const {
		return mInternalResourceTable;
	}

	Frame::Frame(const std::filesystem::path& path) {
		mPath = path;
		mDevice = Device::Disk();
	}

	entt::entity Frame::SpawnDefaultCamera(Camera** cameraOut) {
		auto cameraNode = CreateEntity();
		auto& camera = Emplace<Camera>(cameraNode);

		if (cameraOut)
			*cameraOut = &camera;

		return cameraNode;
	}

	entt::entity Frame::CreateEntity(entt::entity parent) {
		auto e = mRegistry.create();
		mRegistry.emplace<HierarchyData>(e);
		AddChild(parent, e);
		return e;
	}

	void Frame::Destroy(entt::entity ent) {
		Orphan(ent);

		for (entt::entity child = GetFirstChild(ent); 
			child != entt::null; 
			child = GetNext(child)) 
			Destroy(child);

		mRegistry.destroy(ent);
	}

	Frame::Frame() {
		mRoot = mRegistry.create();
		mRegistry.emplace<HierarchyData>(mRoot);
	}

	entt::meta_type Frame::GetType() const {
		return entt::resolve<Frame>();
	}

	entt::meta_any Frame::GetSourceMeta() const {
		return mPath;
	}

	std::filesystem::path Frame::GetPath() const {
		return mPath;
	}

	void Frame::BinarySerialize(std::ostream& output, IDependencyResolver* dependencies) {
		throw std::runtime_error("Cannot serialize! Use FrameIO instead!");
	}

	void Frame::BinaryDeserialize(std::istream& input, const IDependencyResolver* dependencies) {
		throw std::runtime_error("Cannot deserialize! Use FrameIO instead!");
	}

	void Frame::BinarySerializeReference(
		const std::filesystem::path& workingPath, 
		cereal::PortableBinaryOutputArchive& output) {
	
		auto relativePath = std::filesystem::relative(mPath, workingPath);
		output(relativePath.string());
	}

	void Frame::BinaryDeserializeReference(
		const std::filesystem::path& workingPath,
		cereal::PortableBinaryInputArchive& input) {

		std::string src;
		input(src);
		auto relativeSrc = std::filesystem::path(src);
		mPath = workingPath / relativeSrc;
	}

	BarrierOut Frame::MoveAsync(Device device, Context context) {
		Barrier barrier;
		return barrier;
	}

	Handle<IResource> Frame::MoveIntoHandle() {
		return Handle<Frame>(std::move(*this)).DownCast<IResource>();
	}

	void Frame::DuplicateSubframe(Frame& subframe, entt::entity e) {
		std::unordered_map<entt::entity, entt::entity> old_to_new;

		auto root = subframe.GetRoot();
		old_to_new[root, e];

		mRegistry.each([root, &old_to_new, &registry = mRegistry](entt::entity e) {
			if (e != root) {
				auto newE = registry.create();
				old_to_new[e] = newE;
			}
		});

		// Copy everything!
		ForEachCopyableType([&old_to_new, this, &subframe](const CopyableType type) {
			type->CopySet(subframe.Registry(), Registry(), old_to_new);
		});
	}

	Handle<IResource> Frame::GetResourceAbstract(entt::entity e) const {
		auto it = mEntityToResource.find(e);

		if (it == mEntityToResource.end())
			return nullptr;

		return it->second;
	}

	entt::entity Frame::GetEntity(const std::string& name) const {
		auto it = mNameToEntity.find(name);

		if (it != mNameToEntity.end()) {
			return it->second;
		} else {
			return entt::null;
		}
	}
}